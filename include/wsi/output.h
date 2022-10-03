#ifndef WSI_INCLUDE_OUTPUT_H
#define WSI_INCLUDE_OUTPUT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef WsiResult (*PFN_wsiCreateOutput)(WsiPlatform platform, WsiOutput *output);

WsiResult
wsiCreateOutput(WsiPlatform platform, WsiOutput *pOutput);

#ifdef __cplusplus
}
#endif

#endif
