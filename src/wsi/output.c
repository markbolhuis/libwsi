#include <dlfcn.h>

#include "wsi/output.h"

extern void *g_handle;

WsiResult
wsiCreateOutput(WsiPlatform platform, WsiOutput *pOutput)
{
    PFN_wsiCreateOutput sym
        = (PFN_wsiCreateOutput)dlsym(g_handle, "wsiCreateOutput");
    return sym(platform, pOutput);
}
