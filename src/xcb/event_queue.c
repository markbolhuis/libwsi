#include <xcb/xcb.h>

#include "wsi/event_queue.h"

WsiResult
wsiCreateEventQueue(WsiPlatform platform, WsiEventQueue *pEventQueue)
{
    return WSI_SUCCESS;
}

void
wsiDestroyEventQueue(WsiEventQueue eventQueue)
{

}

WsiResult
wsiPollEventQueue(WsiEventQueue eventQueue)
{
    return WSI_ERROR_NOT_IMPLEMENTED;
}
