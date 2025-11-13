#include <linux/string.h>
#include "perfmgr_profile.h"

int perfmgr_get_profile_idx(const char *name)
{
    if (!name)
        return -1;

    if (!strcmp(name, "normal"))
        return 0;
    if (!strcmp(name, "boost"))
        return 1;

    return -1;
}
