#undef TRACE_SYSTEM
#define TRACE_SYSTEM perfmgr

#if !defined(_TRACE_PERFMGR_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_PERFMGR_H

#include <linux/tracepoint.h>

TRACE_EVENT(perfmgr_boost_event,
    TP_PROTO(const char *tag, int val),
    TP_ARGS(tag, val),
    TP_STRUCT__entry(
        __string(tag, tag)
        __field(int, val)
    ),
    TP_fast_assign(
        __assign_str(tag, tag);
        __entry->val = val;
    ),
    TP_printk("tag=%s val=%d", __get_str(tag), __entry->val)
);

#endif

#include <trace/define_trace.h>
