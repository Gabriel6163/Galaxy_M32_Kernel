/*
 * OrkGame I/O Scheduler — Production-ready for eMMC 5.1
 * Kernel 4.14.356 compatible (Samsung Galaxy M32 - SM-M325FV)
 *
 * FINAL CORRECTED VERSION (compatible with your kernel tree):
 * - Uses lazy per-request allocation (ORK_MANAGE_RQ = 1) because your
 *   include/linux/elevator.h does not expose per-rq init/exit fields.
 * - All lazy alloc/free sites are conditioned on ORK_MANAGE_RQ to avoid
 *   double-free if you later switch to per-rq callbacks.
 * - REQ_PRIO usage guarded to avoid build errors on kernels where it is absent.
 * - Sysfs stores apply changes immediately to the elevator_queue instance passed
 *   by the block core (ork_apply_sysfs_to_dd).
 * - Other fixes and safeguards applied as discussed.
 *
 * Author: OrkGabb (finalized & adapted)
 * License: GPL v2
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/compiler.h>
#include <linux/rbtree.h>
#include <linux/jiffies.h>
#include <linux/sysfs.h>
#include <asm/barrier.h>

/* ========= Choose per-request management model =========
 *  ORK_MANAGE_RQ = 1 -> lazy allocation/free inside the elevator (compatible
 *                       with kernels that do not expose per-rq init/exit hooks).
 *  ORK_MANAGE_RQ = 0 -> rely on elevator_type-level init_rq/exit_rq callbacks
 *                       (only use if your kernel's struct elevator_type supports them).
 *
 * Based on your kernel's include/linux/elevator.h (no per-rq fields), we set:
 */
#define ORK_MANAGE_RQ 1

/* Optional compile-time features */
#define ORK_ENABLE_CRITICAL_QUEUE 0
#define ORK_ENABLE_SYNC_HINTS 1

/* ============================================================================
 * Tunables (balanced gaming defaults)
 * ============================================================================ */

/* Base latency (ms) */
static int read_expire_ms = 200;
static int write_expire_ms = 4000;
static int critical_read_expire_ms = 30;

/* Burst/Turbo */
static int burst_window_ms = 80;
static int burst_threshold = 8;
static int burst_decay_ms = 150;
static int burst_hysteresis_ms = 50;
static int turbo_read_boost = 3;

/* Request classification (KB) */
static int small_read_kb = 16;
static int large_read_kb = 128;
static int tiny_write_kb = 4;

/* Scheduling */
static int fifo_batch = 24;
static int writes_starved_ratio = 3;
static int writes_starved_burst = 6;
static int small_read_scan_depth = 24;

/* Write management */
static int write_throttle_threshold = 32;
static int write_coalesce_delay_ms = 5;
static int max_coalesced_writes = 8;
static int coalesce_sector_threshold = 256;

/* Anti-starvation */
static int max_request_age_ms = 1000;
static int aging_boost_factor = 2;

/* Sequential detection */
static int seq_threshold_sectors = 128;
static int seq_boost_factor = 2;

/* ============================================================================
 * Data structures
 * ============================================================================ */

enum ork_class {
    ORK_CRIT_READ = 0,
    ORK_SMALL_READ,
    ORK_LARGE_READ,
    ORK_NORMAL_WRITE,
    ORK_TINY_WRITE
};

struct ork_rq_data {
    unsigned long enqueue_time;
    u8 class;
    u8 in_coalesce : 1;
    u8 _pad : 7;
} __attribute__((packed));

struct ork_burst {
    unsigned long window_start;
    unsigned long burst_deadline;
    unsigned long last_exit_time;
    u16 read_count;
    u8 active;
    u8 in_cooldown;
    u8 _pad[10];
};

struct ork_pattern {
    sector_t last_sector;
    u32 seq_score;
    u8 is_sequential;
    u8 last_was_large;
    u16 _pad;
};

struct orkgame_data {
    struct rb_root sort_list[2];
    struct request *next_rq[2];
    u32 batching;
    u32 starved;
    int fifo_expire[2];
    u8 writes_starved;
    u8 front_merges;
    u8 write_throttle_active;
    unsigned int write_fifo_count;
    u8 _pad1[9];

    struct list_head fifo_list[2];
    struct ork_burst burst;

    struct list_head coalesce_list;
    unsigned long coalesce_deadline;
    unsigned long oldest_rq_time;
    u16 coalesce_count;
    u8 fifo_batch;
    u8 orig_writes_starved;
    int fifo_critical_expire;
    int orig_read_expire;

    struct ork_pattern pattern;

#if ORK_ENABLE_CRITICAL_QUEUE
    struct list_head critical_fifo;
    u16 critical_count;
#endif
};

/* Forward declarations */
static void ork_merged_request(struct request_queue *q, struct request *req, enum elv_merge type);
static void ork_merged_requests(struct request_queue *q, struct request *req, struct request *next);
static int ork_dispatch_requests(struct request_queue *q, int force);
static void ork_add_request(struct request_queue *q, struct request *rq);
static struct request *ork_former_request(struct request_queue *q, struct request *rq);
static int ork_init_queue(struct request_queue *q, struct elevator_type *e);
static void ork_exit_queue(struct elevator_queue *e);
static struct request *ork_find_best_read(struct orkgame_data *dd);
static void ork_flush_coalesced(struct orkgame_data *dd);

/* Helper that applies global tunables to an elevator_queue instance */
static void ork_apply_sysfs_to_dd(struct elevator_queue *e)
{
    struct orkgame_data *dd;
    if (!e || !e->elevator_data)
        return;
    dd = e->elevator_data;

    dd->fifo_expire[READ] = msecs_to_jiffies(read_expire_ms);
    dd->fifo_expire[WRITE] = msecs_to_jiffies(write_expire_ms);
    dd->fifo_critical_expire = msecs_to_jiffies(critical_read_expire_ms);
    dd->fifo_batch = fifo_batch;
    dd->writes_starved = writes_starved_ratio;
    dd->orig_writes_starved = dd->writes_starved;
}

/* ============================================================================
 * Inline helpers
 * ============================================================================ */

static __always_inline struct rb_root *ork_rb_root(struct orkgame_data *dd, struct request *rq)
{
    return &dd->sort_list[rq_data_dir(rq)];
}

static __always_inline struct request *ork_latter_request(struct request *rq)
{
    struct rb_node *node = rb_next(&rq->rb_node);
    return likely(node) ? rb_entry_rq(node) : NULL;
}

static __always_inline struct ork_rq_data *ork_rq_data(struct request *rq)
{
    return (struct ork_rq_data *)rq->elv.priv[0];
}

static __always_inline bool ork_fifo_empty(struct orkgame_data *dd, int ddir)
{
    return list_empty(&dd->fifo_list[ddir]);
}

static __always_inline unsigned int ork_request_kb(struct request *rq)
{
    return blk_rq_bytes(rq) >> 10;
}

static __always_inline bool ork_is_aged(struct ork_rq_data *rqd)
{
    if (unlikely(!rqd))
        return false;
    return time_after(jiffies, rqd->enqueue_time + msecs_to_jiffies(max_request_age_ms));
}

static __always_inline bool ork_fifo_expired(struct orkgame_data *dd, int ddir)
{
    struct request *rq;
    if (unlikely(ork_fifo_empty(dd, ddir)))
        return false;
    rq = rq_entry_fifo(dd->fifo_list[ddir].next);
    return time_after_eq(jiffies, rq->fifo_time);
}

static __always_inline unsigned int ork_write_depth(struct orkgame_data *dd)
{
    return dd->write_fifo_count + dd->coalesce_count;
}

static inline bool ork_requests_adjacent(struct request *rq1, struct request *rq2)
{
    sector_t end1 = blk_rq_pos(rq1) + blk_rq_sectors(rq1);
    sector_t start2 = blk_rq_pos(rq2);
    sector_t gap = (start2 > end1) ? (start2 - end1) : (end1 - start2);
    return (gap <= coalesce_sector_threshold);
}

/* ============================================================================
 * Core logic
 * ============================================================================ */

static __always_inline enum ork_class ork_classify(struct request *rq)
{
    const int dir = rq_data_dir(rq);
    const unsigned int kb = ork_request_kb(rq);

    if (likely(dir == READ)) {
        if (kb <= (small_read_kb >> 1)) return ORK_CRIT_READ;
        if (kb <= small_read_kb) return ORK_SMALL_READ;
        return ORK_LARGE_READ;
    }
    return (kb <= tiny_write_kb) ? ORK_TINY_WRITE : ORK_NORMAL_WRITE;
}

static __always_inline int ork_priority(struct request *rq, struct orkgame_data *dd)
{
    struct ork_rq_data *rqd = ork_rq_data(rq);
    int prio = rqd ? rqd->class * 10 : 20;

    if (dd->burst.active && rq_data_dir(rq) == READ)
        prio += turbo_read_boost * 10;

    if (rqd && ork_is_aged(rqd))
        prio += aging_boost_factor * 15;

    if (dd->pattern.is_sequential && rqd && rqd->class == ORK_LARGE_READ)
        prio += seq_boost_factor * 8;

#if ORK_ENABLE_SYNC_HINTS
    if (rq->cmd_flags & REQ_SYNC)
        prio += 10;
#ifdef REQ_PRIO
    if (rq->cmd_flags & REQ_PRIO)
        prio += 20;
#endif
#endif

    return prio;
}

static inline void ork_update_pattern(struct orkgame_data *dd, struct request *rq)
{
    struct ork_pattern *p = &dd->pattern;
    sector_t pos = blk_rq_pos(rq);
    sector_t delta;
    unsigned int kb = ork_request_kb(rq);

    if (unlikely(rq_data_dir(rq) != READ))
        return;

    if (unlikely(p->last_sector == 0)) {
        p->last_sector = pos;
        p->last_was_large = (kb >= large_read_kb);
        return;
    }

    delta = (pos > p->last_sector) ? (pos - p->last_sector) : (p->last_sector - pos);

    bool is_large = (kb >= large_read_kb);
    if (delta <= seq_threshold_sectors && is_large && p->last_was_large) {
        p->seq_score = min_t(u32, p->seq_score + 8, 255);
        p->is_sequential = (p->seq_score > 96);
    } else {
        p->seq_score = (p->seq_score > 3) ? (p->seq_score - 3) : 0;
        p->is_sequential = 0;
    }

    p->last_sector = pos;
    p->last_was_large = is_large;
}

static __always_inline void ork_burst_tick(struct orkgame_data *dd, struct request *rq)
{
    struct ork_burst *b = &dd->burst;
    unsigned long now = jiffies;

    if (unlikely(rq_data_dir(rq) != READ))
        return;

    if (b->in_cooldown && time_after(now, b->last_exit_time + msecs_to_jiffies(burst_hysteresis_ms)))
        b->in_cooldown = 0;

    if (unlikely(!b->window_start || time_after(now, b->window_start + msecs_to_jiffies(burst_window_ms)))) {
        b->window_start = now;
        b->read_count = 0;
    }

    b->read_count++;

    if (unlikely(!b->active && b->read_count >= burst_threshold)) {
        if (b->in_cooldown) return;
        b->active = 1;
        b->burst_deadline = now + msecs_to_jiffies(burst_decay_ms);
        dd->orig_writes_starved = dd->writes_starved;
        dd->orig_read_expire = dd->fifo_expire[READ];
        dd->writes_starved = writes_starved_burst;
        dd->fifo_expire[READ] = msecs_to_jiffies(critical_read_expire_ms);
    }
}

static inline void ork_burst_decay(struct orkgame_data *dd)
{
    struct ork_burst *b = &dd->burst;
    unsigned long now = jiffies;
    if (likely(!b->active)) return;
    if (time_after_eq(now, b->burst_deadline)) {
        b->active = 0;
        b->in_cooldown = 1;
        b->last_exit_time = now;
        dd->writes_starved = dd->orig_writes_starved;
        dd->fifo_expire[READ] = dd->orig_read_expire;
        b->read_count = 0;
    }
}

/* Coalesce helpers */
static inline bool ork_should_coalesce(struct orkgame_data *dd, struct ork_rq_data *rqd)
{
    return (rqd && rqd->class == ORK_TINY_WRITE && dd->coalesce_count < max_coalesced_writes && !ork_is_aged(rqd));
}

static inline void ork_coalesce_write(struct orkgame_data *dd, struct request *rq)
{
    struct ork_rq_data *rqd = ork_rq_data(rq);

#if ORK_MANAGE_RQ
    if (rqd)
        rqd->in_coalesce = 1;
#endif

    list_add_tail(&rq->queuelist, &dd->coalesce_list);

    if (unlikely(++dd->coalesce_count == 1))
        dd->coalesce_deadline = jiffies + msecs_to_jiffies(write_coalesce_delay_ms);
}

/* Flush coalesced writes with restart-on-merge semantics */
static void ork_flush_coalesced(struct orkgame_data *dd)
{
    struct request *rq, *next, *neighbor;
    struct list_head *head = &dd->coalesce_list;

    if (dd->coalesce_count == 0)
        return;

restart_scan:
    list_for_each_entry_safe(rq, next, head, queuelist) {
        if (rq->queuelist.next == head)
            continue;

        neighbor = list_entry(rq->queuelist.next, struct request, queuelist);

        if (ork_requests_adjacent(rq, neighbor)) {
            if (blk_attempt_req_merge(rq->q, rq, neighbor)) {
                /* neighbor merged into rq: free neighbor private data if managed here */
#if ORK_MANAGE_RQ
                if (neighbor->elv.priv[0]) {
                    kfree(neighbor->elv.priv[0]);
                    neighbor->elv.priv[0] = NULL;
                }
#endif
                list_del_init(&neighbor->queuelist);
                if (dd->coalesce_count)
                    dd->coalesce_count--;
                goto restart_scan;
            }
        }
    }

    /* Move remaining coalesced requests into write FIFO */
    list_for_each_entry_safe(rq, next, head, queuelist) {
#if ORK_MANAGE_RQ
        struct ork_rq_data *rqd = ork_rq_data(rq);
        if (rqd) rqd->in_coalesce = 0;
#endif
        list_del_init(&rq->queuelist);
        list_add_tail(&rq->queuelist, &dd->fifo_list[WRITE]);
        dd->write_fifo_count++;
    }

    dd->coalesce_count = 0;
}

/* Throttle logic */
static inline void ork_update_write_throttle(struct orkgame_data *dd)
{
    unsigned int depth = ork_write_depth(dd);
    if (depth > (unsigned)write_throttle_threshold && !dd->write_throttle_active) {
        dd->write_throttle_active = 1;
        dd->writes_starved = dd->orig_writes_starved * 2;
    } else if (depth <= (unsigned)(write_throttle_threshold / 2) && dd->write_throttle_active) {
        dd->write_throttle_active = 0;
        dd->writes_starved = dd->orig_writes_starved;
    }
}

/* RB helpers and read selection */
static inline void ork_add_rq_rb(struct orkgame_data *dd, struct request *rq) { elv_rb_add(ork_rb_root(dd, rq), rq); }

static inline void ork_del_rq_rb(struct orkgame_data *dd, struct request *rq)
{
    const int dir = rq_data_dir(rq);
    if (unlikely(dd->next_rq[dir] == rq))
        dd->next_rq[dir] = ork_latter_request(rq);
    elv_rb_del(ork_rb_root(dd, rq), rq);
}

static struct request *ork_find_best_read(struct orkgame_data *dd)
{
    struct request *best = NULL, *rq;
    struct ork_rq_data *rqd_best = NULL, *rqd;
    struct list_head *pos;
    int best_prio = -1000, prio;
    int scanned = 0;

    if (unlikely(ork_fifo_empty(dd, READ)))
        return NULL;

    best = rq_entry_fifo(dd->fifo_list[READ].next);
    rqd_best = ork_rq_data(best);

    if ((rqd_best && ork_is_aged(rqd_best)) || (rqd_best && rqd_best->class == ORK_CRIT_READ))
        return best;

    best_prio = ork_priority(best, dd);

    list_for_each(pos, &dd->fifo_list[READ]) {
        if (++scanned > small_read_scan_depth) break;
        rq = rq_entry_fifo(pos);
        if (unlikely(list_empty(&rq->queuelist) || rq == best)) continue;
        rqd = ork_rq_data(rq);
        if (unlikely(rqd && ork_is_aged(rqd))) return rq;
        prio = ork_priority(rq, dd);
        if (prio > best_prio) {
            best = rq;
            best_prio = prio;
            rqd_best = rqd;
        }
    }

    return best;
}

/* ============================================================================
 * Elevator operations
 * ============================================================================ */

static void ork_add_request(struct request_queue *q, struct request *rq)
{
    struct orkgame_data *dd = q->elevator->elevator_data;
    struct ork_rq_data *rqd;
    const int dir = rq_data_dir(rq);

    if (unlikely(!dd)) return;

#if ORK_MANAGE_RQ
    if (unlikely(!rq->elv.priv[0]))
        rq->elv.priv[0] = kzalloc_node(sizeof(struct ork_rq_data), GFP_ATOMIC, q->node);
#endif

    rqd = ork_rq_data(rq);
    if (unlikely(!rqd)) {
        list_add_tail(&rq->queuelist, &dd->fifo_list[dir]);
        if (dir == WRITE) dd->write_fifo_count++;
        return;
    }

    rqd->enqueue_time = jiffies;
    rqd->class = ork_classify(rq);

#if ORK_MANAGE_RQ
    rqd->in_coalesce = 0;
#endif

    ork_add_rq_rb(dd, rq);

    rq->fifo_time = jiffies + ((rqd->class == ORK_CRIT_READ) ? dd->fifo_critical_expire : dd->fifo_expire[dir]);

#if ORK_ENABLE_CRITICAL_QUEUE
    if (dir == READ && rqd->class == ORK_CRIT_READ) {
        list_add_tail(&rq->queuelist, &dd->critical_fifo);
        dd->critical_count++;
        ork_update_pattern(dd, rq);
        ork_burst_tick(dd, rq);
        return;
    }
#endif

    if (unlikely(dir == WRITE && ork_should_coalesce(dd, rqd))) {
        ork_coalesce_write(dd, rq);
        return;
    }

    list_add_tail(&rq->queuelist, &dd->fifo_list[dir]);
    if (dir == WRITE) dd->write_fifo_count++;

    ork_update_pattern(dd, rq);
    ork_burst_tick(dd, rq);

    if (unlikely(!dd->oldest_rq_time || time_before(rqd->enqueue_time, dd->oldest_rq_time)))
        dd->oldest_rq_time = rqd->enqueue_time;
}

static void ork_merged_request(struct request_queue *q, struct request *req, enum elv_merge type)
{
    struct orkgame_data *dd = q->elevator->elevator_data;
    if (unlikely(!dd)) return;
    if (unlikely(type == ELEVATOR_FRONT_MERGE)) {
        elv_rb_del(ork_rb_root(dd, req), req);
        ork_add_rq_rb(dd, req);
    }
}

static void ork_merged_requests(struct request_queue *q, struct request *req, struct request *next)
{
    struct orkgame_data *dd = q->elevator->elevator_data;
    struct ork_rq_data *rqd_next = NULL;

    if (unlikely(!dd || !next)) return;

    if (!list_empty(&req->queuelist) && !list_empty(&next->queuelist)) {
        if (time_before((unsigned long)next->fifo_time, (unsigned long)req->fifo_time)) {
            list_move(&req->queuelist, &next->queuelist);
            req->fifo_time = next->fifo_time;
        }
    }

    if (!list_empty(&next->queuelist)) {
#if ORK_MANAGE_RQ
        rqd_next = ork_rq_data(next);
#endif
        if (rq_data_dir(next) == WRITE) {
#if ORK_MANAGE_RQ
            if (!rqd_next || !rqd_next->in_coalesce) {
                if (dd->write_fifo_count) dd->write_fifo_count--;
            }
#else
            if (dd->write_fifo_count) dd->write_fifo_count--;
#endif
        }
        list_del_init(&next->queuelist);
    }

#if ORK_MANAGE_RQ
    if (next->elv.priv[0]) { kfree(next->elv.priv[0]); next->elv.priv[0] = NULL; }
#endif

    ork_del_rq_rb(dd, next);
}

static inline void ork_move_to_dispatch(struct orkgame_data *dd, struct request *rq)
{
    struct ork_rq_data *rqd = ork_rq_data(rq);
    int dir = rq_data_dir(rq);

    if (!list_empty(&rq->queuelist)) {
#if ORK_MANAGE_RQ
        if (dir == WRITE && (!rqd || !rqd->in_coalesce) && dd->write_fifo_count)
            dd->write_fifo_count--;
#else
        if (dir == WRITE && dd->write_fifo_count)
            dd->write_fifo_count--;
#endif
        list_del_init(&rq->queuelist);
    }

    ork_del_rq_rb(dd, rq);
    elv_dispatch_add_tail(rq->q, rq);
}

static void ork_move_request(struct orkgame_data *dd, struct request *rq)
{
    const int dir = rq_data_dir(rq);
    dd->next_rq[READ] = NULL;
    dd->next_rq[WRITE] = NULL;
    dd->next_rq[dir] = ork_latter_request(rq);
    ork_move_to_dispatch(dd, rq);
}

static int ork_dispatch_requests(struct request_queue *q, int force)
{
    struct orkgame_data *dd = q->elevator->elevator_data;
    struct request *rq;
    int dir;

    if (unlikely(!dd)) return 0;

#if ORK_ENABLE_CRITICAL_QUEUE
    if (dd->critical_count > 0) {
        rq = rq_entry_fifo(dd->critical_fifo.next);
        dd->critical_count--;
        goto dispatch_now;
    }
#endif

    if (unlikely(dd->coalesce_count && time_after_eq(jiffies, dd->coalesce_deadline)))
        ork_flush_coalesced(dd);

    ork_update_write_throttle(dd);

    if (unlikely(dd->burst.active)) ork_burst_decay(dd);

    if (likely(dd->batching < dd->fifo_batch)) {
        rq = dd->next_rq[WRITE] ? dd->next_rq[WRITE] : dd->next_rq[READ];
        if (rq) goto dispatch;
    }

    {
        const int reads = !ork_fifo_empty(dd, READ);
        const int writes = !ork_fifo_empty(dd, WRITE);

        if (likely(reads)) {
            if (unlikely(writes && (dd->starved++ >= dd->writes_starved))) {
                dir = WRITE; dd->starved = 0; goto find_request;
            }
            dir = READ; goto find_request;
        }

        if (writes) { dir = WRITE; dd->starved = 0; goto find_request; }
    }

    return 0;

find_request:
    if (dir == READ) rq = ork_find_best_read(dd);
    else rq = ork_fifo_expired(dd, WRITE) || !dd->next_rq[WRITE] ? rq_entry_fifo(dd->fifo_list[WRITE].next) : dd->next_rq[WRITE];

    if (unlikely(!rq)) return 0;
    dd->batching = 0;

dispatch:
    dd->batching++;
#if ORK_ENABLE_CRITICAL_QUEUE
dispatch_now:
#endif
    ork_move_request(dd, rq);
    return 1;
}

/* ============================================================================
 * Initialization & cleanup
 * ============================================================================ */

static int ork_init_queue(struct request_queue *q, struct elevator_type *e)
{
    struct orkgame_data *dd;
    struct elevator_queue *eq;

    eq = elevator_alloc(q, e);
    if (unlikely(!eq)) return -ENOMEM;

    dd = kzalloc(sizeof(*dd), GFP_KERNEL);
    if (unlikely(!dd)) { kobject_put(&eq->kobj); return -ENOMEM; }

    eq->elevator_data = dd;

    INIT_LIST_HEAD(&dd->fifo_list[READ]);
    INIT_LIST_HEAD(&dd->fifo_list[WRITE]);
    INIT_LIST_HEAD(&dd->coalesce_list);
    dd->sort_list[READ] = RB_ROOT;
    dd->sort_list[WRITE] = RB_ROOT;

#if ORK_ENABLE_CRITICAL_QUEUE
    INIT_LIST_HEAD(&dd->critical_fifo);
    dd->critical_count = 0;
#endif

    dd->fifo_expire[READ] = msecs_to_jiffies(read_expire_ms);
    dd->fifo_expire[WRITE] = msecs_to_jiffies(write_expire_ms);
    dd->fifo_critical_expire = msecs_to_jiffies(critical_read_expire_ms);
    dd->fifo_batch = fifo_batch;
    dd->writes_starved = writes_starved_ratio;
    dd->front_merges = 1;
    dd->write_throttle_active = 0;

    dd->orig_writes_starved = dd->writes_starved;
    dd->orig_read_expire = dd->fifo_expire[READ];

    dd->coalesce_count = 0;
    dd->write_fifo_count = 0;

    spin_lock_irq(q->queue_lock);
    q->elevator = eq;
    spin_unlock_irq(q->queue_lock);

    return 0;
}

static void ork_exit_queue(struct elevator_queue *e)
{
    struct orkgame_data *dd = e->elevator_data;

    BUG_ON(!list_empty(&dd->fifo_list[READ]));
    BUG_ON(!list_empty(&dd->fifo_list[WRITE]));
    BUG_ON(!list_empty(&dd->coalesce_list));
#if ORK_ENABLE_CRITICAL_QUEUE
    BUG_ON(!list_empty(&dd->critical_fifo));
#endif

    kfree(dd);
}

static struct request *ork_former_request(struct request_queue *q, struct request *rq)
{
    struct rb_node *rbprev = rb_prev(&rq->rb_node);
    return rbprev ? rb_entry_rq(rbprev) : NULL;
}

/* ============================================================================
 * Sysfs tunables: show/store that apply to the given elevator_queue
 * ============================================================================ */

#define ORK_SHOW(name, var) \
static ssize_t ork_##name##_show(struct elevator_queue *e, char *page) { return scnprintf(page, PAGE_SIZE, "%d\n", var); }

#define ORK_STORE_SIMPLE(name, var, min_val, max_val) \
static ssize_t ork_##name##_store(struct elevator_queue *e, const char *page, size_t count) { \
    int val; if (kstrtoint(page, 10, &val)) return -EINVAL; \
    if (val < (min_val) || val > (max_val)) return -EINVAL; \
    var = val; smp_wmb(); if (e) ork_apply_sysfs_to_dd(e); return count; \
}

ORK_SHOW(read_expire_ms, read_expire_ms)
ORK_STORE_SIMPLE(read_expire_ms, read_expire_ms, 10, 5000)
ORK_SHOW(write_expire_ms, write_expire_ms)
ORK_STORE_SIMPLE(write_expire_ms, write_expire_ms, 100, 30000)
ORK_SHOW(critical_read_expire_ms, critical_read_expire_ms)
ORK_STORE_SIMPLE(critical_read_expire_ms, critical_read_expire_ms, 5, 500)
ORK_SHOW(burst_window_ms, burst_window_ms)
ORK_STORE_SIMPLE(burst_window_ms, burst_window_ms, 20, 500)
ORK_SHOW(burst_threshold, burst_threshold)
ORK_STORE_SIMPLE(burst_threshold, burst_threshold, 2, 50)
ORK_SHOW(burst_decay_ms, burst_decay_ms)
ORK_STORE_SIMPLE(burst_decay_ms, burst_decay_ms, 50, 2000)
ORK_SHOW(burst_hysteresis_ms, burst_hysteresis_ms)
ORK_STORE_SIMPLE(burst_hysteresis_ms, burst_hysteresis_ms, 0, 500)
ORK_SHOW(turbo_read_boost, turbo_read_boost)
ORK_STORE_SIMPLE(turbo_read_boost, turbo_read_boost, 0, 10)
ORK_SHOW(small_read_kb, small_read_kb)
ORK_STORE_SIMPLE(small_read_kb, small_read_kb, 4, 128)
ORK_SHOW(large_read_kb, large_read_kb)
ORK_STORE_SIMPLE(large_read_kb, large_read_kb, 32, 1024)
ORK_SHOW(tiny_write_kb, tiny_write_kb)
ORK_STORE_SIMPLE(tiny_write_kb, tiny_write_kb, 1, 64)
ORK_SHOW(fifo_batch, fifo_batch)
ORK_STORE_SIMPLE(fifo_batch, fifo_batch, 1, 128)
ORK_SHOW(writes_starved_ratio, writes_starved_ratio)
ORK_STORE_SIMPLE(writes_starved_ratio, writes_starved_ratio, 1, 20)
ORK_SHOW(writes_starved_burst, writes_starved_burst)
ORK_STORE_SIMPLE(writes_starved_burst, writes_starved_burst, 1, 20)
ORK_SHOW(small_read_scan_depth, small_read_scan_depth)
ORK_STORE_SIMPLE(small_read_scan_depth, small_read_scan_depth, 4, 128)
ORK_SHOW(write_throttle_threshold, write_throttle_threshold)
ORK_STORE_SIMPLE(write_throttle_threshold, write_throttle_threshold, 8, 256)
ORK_SHOW(write_coalesce_delay_ms, write_coalesce_delay_ms)
ORK_STORE_SIMPLE(write_coalesce_delay_ms, write_coalesce_delay_ms, 1, 100)
ORK_SHOW(max_coalesced_writes, max_coalesced_writes)
ORK_STORE_SIMPLE(max_coalesced_writes, max_coalesced_writes, 2, 64)
ORK_SHOW(coalesce_sector_threshold, coalesce_sector_threshold)
ORK_STORE_SIMPLE(coalesce_sector_threshold, coalesce_sector_threshold, 8, 2048)
ORK_SHOW(max_request_age_ms, max_request_age_ms)
ORK_STORE_SIMPLE(max_request_age_ms, max_request_age_ms, 100, 10000)
ORK_SHOW(aging_boost_factor, aging_boost_factor)
ORK_STORE_SIMPLE(aging_boost_factor, aging_boost_factor, 0, 10)
ORK_SHOW(seq_threshold_sectors, seq_threshold_sectors)
ORK_STORE_SIMPLE(seq_threshold_sectors, seq_threshold_sectors, 16, 1024)
ORK_SHOW(seq_boost_factor, seq_boost_factor)
ORK_STORE_SIMPLE(seq_boost_factor, seq_boost_factor, 0, 10)

static struct elv_fs_entry ork_attrs[] = {
    __ATTR(read_expire_ms, 0644, ork_read_expire_ms_show, ork_read_expire_ms_store),
    __ATTR(write_expire_ms, 0644, ork_write_expire_ms_show, ork_write_expire_ms_store),
    __ATTR(critical_read_expire_ms, 0644, ork_critical_read_expire_ms_show, ork_critical_read_expire_ms_store),
    __ATTR(burst_window_ms, 0644, ork_burst_window_ms_show, ork_burst_window_ms_store),
    __ATTR(burst_threshold, 0644, ork_burst_threshold_show, ork_burst_threshold_store),
    __ATTR(burst_decay_ms, 0644, ork_burst_decay_ms_show, ork_burst_decay_ms_store),
    __ATTR(burst_hysteresis_ms, 0644, ork_burst_hysteresis_ms_show, ork_burst_hysteresis_ms_store),
    __ATTR(turbo_read_boost, 0644, ork_turbo_read_boost_show, ork_turbo_read_boost_store),
    __ATTR(small_read_kb, 0644, ork_small_read_kb_show, ork_small_read_kb_store),
    __ATTR(large_read_kb, 0644, ork_large_read_kb_show, ork_large_read_kb_store),
    __ATTR(tiny_write_kb, 0644, ork_tiny_write_kb_show, ork_tiny_write_kb_store),
    __ATTR(fifo_batch, 0644, ork_fifo_batch_show, ork_fifo_batch_store),
    __ATTR(writes_starved_ratio, 0644, ork_writes_starved_ratio_show, ork_writes_starved_ratio_store),
    __ATTR(writes_starved_burst, 0644, ork_writes_starved_burst_show, ork_writes_starved_burst_store),
    __ATTR(small_read_scan_depth, 0644, ork_small_read_scan_depth_show, ork_small_read_scan_depth_store),
    __ATTR(write_throttle_threshold, 0644, ork_write_throttle_threshold_show, ork_write_throttle_threshold_store),
    __ATTR(write_coalesce_delay_ms, 0644, ork_write_coalesce_delay_ms_show, ork_write_coalesce_delay_ms_store),
    __ATTR(max_coalesced_writes, 0644, ork_max_coalesced_writes_show, ork_max_coalesced_writes_store),
    __ATTR(coalesce_sector_threshold, 0644, ork_coalesce_sector_threshold_show, ork_coalesce_sector_threshold_store),
    __ATTR(max_request_age_ms, 0644, ork_max_request_age_ms_show, ork_max_request_age_ms_store),
    __ATTR(aging_boost_factor, 0644, ork_aging_boost_factor_show, ork_aging_boost_factor_store),
    __ATTR(seq_threshold_sectors, 0644, ork_seq_threshold_sectors_show, ork_seq_threshold_sectors_store),
    __ATTR(seq_boost_factor, 0644, ork_seq_boost_factor_show, ork_seq_boost_factor_store),
    __ATTR_NULL
};

/* Elevator registration (do NOT reference elevator_init_rq_fn/exit_rq_fn fields) */
static struct elevator_type iosched_orkgame = {
    .ops.sq = {
        .elevator_merge_fn = NULL,
        .elevator_merged_fn = ork_merged_request,
        .elevator_merge_req_fn = ork_merged_requests,
        .elevator_dispatch_fn = ork_dispatch_requests,
        .elevator_add_req_fn = ork_add_request,
        .elevator_former_req_fn = ork_former_request,
        .elevator_latter_req_fn = elv_rb_latter_request,
        .elevator_init_fn = ork_init_queue,
        .elevator_exit_fn = ork_exit_queue,
    },
    .elevator_attrs = ork_attrs,
    .elevator_name = "orkgame",
    .elevator_owner = THIS_MODULE,
};

static int __init orkgame_init(void)
{
    int ret = elv_register(&iosched_orkgame);
    if (ret) {
        pr_err("OrkGame: failed to register scheduler (%d)\n", ret);
        return ret;
    }
    pr_info("OrkGame I/O scheduler registered (v4.14.356 - production)\n");
    return 0;
}

static void __exit orkgame_exit(void)
{
    elv_unregister(&iosched_orkgame);
    pr_info("OrkGame I/O scheduler unregistered\n");
}

module_init(orkgame_init);
module_exit(orkgame_exit);

MODULE_AUTHOR("OrkGabb");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("OrkGame Final: Production I/O scheduler for gaming on eMMC 5.1");
MODULE_VERSION("1.1-final");
