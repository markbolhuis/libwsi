#include <dlfcn.h>

#include "wsi/output.h"

extern void *g_handle;

WsiResult
wsiEnumerateOutputs(WsiPlatform platform, uint32_t *pCount, WsiOutput *pOutputs)
{
    PFN_wsiEnumerateOutputs sym
        = (PFN_wsiEnumerateOutputs)dlsym(g_handle, "wsiEnumerateOutputs");
    return sym(platform, pCount, pOutputs);
}
