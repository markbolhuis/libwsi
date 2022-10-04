#include <stdlib.h>

#include "wsi/output.h"

#include "platform_priv.h"
#include "output_priv.h"

void *
wsi_output_dlsym(struct wsi_output *output, const char *symbol)
{
    return wsi_platform_dlsym(output->platform, symbol);
}

WsiResult
wsiCreateOutput(
    WsiPlatform platform,
    WsiOutput *pOutput)
{
    struct wsi_output *output = calloc(1, sizeof(struct wsi_output));
    if (!output) {
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    PFN_wsiCreateOutput sym = wsi_platform_dlsym(platform, "wsiCreateOutput");

    enum wsi_result result = sym(platform->platform, &output->output);
    if (result != WSI_SUCCESS) {
        free(output);
        return result;
    }

    output->platform = platform;
    *pOutput = output;
    return WSI_SUCCESS;
}
