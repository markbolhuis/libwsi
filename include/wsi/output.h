#ifndef WSI_INCLUDE_OUTPUT_H
#define WSI_INCLUDE_OUTPUT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef WsiResult (*PFN_wsiEnumerateOutputs)(WsiPlatform platform, uint32_t *pCount, WsiOutput *pOutputs);

WsiResult
wsiEnumerateOutputs(WsiPlatform platform, uint32_t *pCount, WsiOutput *pOutputs);

#ifdef __cplusplus
}
#endif

#endif
