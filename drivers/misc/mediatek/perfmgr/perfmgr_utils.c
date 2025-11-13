#include <linux/kernel.h>
#include "perfmgr_utils.h"

void perfmgr_debug(const char *msg)
{
    pr_debug("[PERFMGR] %s\n", msg);
}
