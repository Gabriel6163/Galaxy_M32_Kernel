#include "perfmgr_trace_event.h"

void perfmgr_trace_boost(const char *tag, int val)
{
    trace_perfmgr_boost_event(tag, val);
}
