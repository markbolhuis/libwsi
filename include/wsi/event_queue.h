#ifndef WSI_INCLUDE_EVENT_QUEUE_H
#define WSI_INCLUDE_EVENT_QUEUE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef WsiResult (*PFN_wsiCreateEventQueue)(WsiPlatform platform, WsiEventQueue *pEventQueue);
typedef void (*PFN_wsiDestroyEventQueue)(WsiEventQueue eventQueue);
typedef WsiResult (*PFN_wsiPollEventQueue)(WsiEventQueue eventQueue);

WsiResult
wsiCreateEventQueue(WsiPlatform platform, WsiEventQueue *pEventQueue);

void
wsiDestroyEventQueue(WsiEventQueue eventQueue);

WsiResult
wsiPollEventQueue(WsiEventQueue eventQueue);

#ifdef __cplusplus
}
#endif

#endif