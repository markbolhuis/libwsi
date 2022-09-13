#ifndef WSI_INCLUDE_EVENT_QUEUE_H
#define WSI_INCLUDE_EVENT_QUEUE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

WsiResult
wsiCreateEventQueue(WsiPlatform platform, WsiEventQueue *pEventQueue);

void
wsiDestroyEventQueue(WsiPlatform platform, WsiEventQueue eventQueue);

WsiResult
wsiPollEventQueue(WsiEventQueue eventQueue);

#ifdef __cplusplus
}
#endif

#endif