#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#define WL_EGL_PLATFORM 1
#define EGL_EGL_PROTOTYPES 0
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "wsi/egl/egl.h"

#include <wayland-util.h>

#include "../platform_priv.h"
#include "../window_priv.h"
#include "egl_priv.h"

// region WL EGL

struct wl_egl_window;

typedef struct wl_egl_window *(*wl_egl_window_create_func_t)(
    struct wl_surface *surface,
    int width, int height);

typedef void (*wl_egl_window_destroy_func_t)(
    struct wl_egl_window *egl_window);

typedef void (*wl_egl_window_resize_func_t)(
    struct wl_egl_window *egl_window,
    int width, int height,
    int dx, int dy);

typedef void (*wl_egl_window_get_attached_size_func_t)(
    struct wl_egl_window *egl_window,
    int *width, int *height);

// endregion

struct wsi_egl {
    void *wl_egl_handle;
    void *egl_handle;

    PFNEGLGETPROCADDRESSPROC              eglGetProcAddress;
    PFNEGLGETPLATFORMDISPLAYPROC          eglGetPlatformDisplay;
    PFNEGLCREATEPLATFORMWINDOWSURFACEPROC eglCreatePlatformWindowSurface;

    wl_egl_window_create_func_t            wl_egl_window_create;
    wl_egl_window_destroy_func_t           wl_egl_window_destroy;
    wl_egl_window_resize_func_t            wl_egl_window_resize;
    wl_egl_window_get_attached_size_func_t wl_egl_window_get_attached_size;
};

void
wsi_window_egl_configure(
    struct wsi_window *window,
    struct wsi_xdg_extent extent)
{
    struct wsi_egl *egl = window->platform->egl;

    if (window->wl_egl_window) {
        egl->wl_egl_window_resize(
            window->wl_egl_window,
            window->current.extent.width,
            window->current.extent.height,
            0,
            0);
    }
    else {
        window->wl_egl_window = egl->wl_egl_window_create(
            window->wl_surface,
            window->current.extent.width,
            window->current.extent.height);
    }
}

#define WL_EGL_LOAD_FUNC(name) \
    egl->name = dlsym(egl->wl_egl_handle, #name); \
    if (!egl->name) { \
        goto err_dlsym_wl_egl; \
    }

enum wsi_result
wsi_egl_load(struct wsi_platform *platform)
{
    void *egl_handle = dlopen("libEGL.so.1", RTLD_NOLOAD | RTLD_LAZY);
    if (!egl_handle) {
        return WSI_SKIPPED;
    }

    struct wsi_egl *egl = calloc(1, sizeof(struct wsi_egl));
    if (!egl) {
        dlclose(egl_handle);
        return WSI_ERROR_OUT_OF_MEMORY;
    }

    egl->egl_handle = egl_handle;

    egl->eglGetProcAddress = dlsym(egl->egl_handle, "eglGetProcAddress");
    if (!egl->eglGetProcAddress) {
        goto err_dlsym_egl;
    }

    egl->wl_egl_handle = dlopen("libwayland-egl.so.1", RTLD_NOW);
    if (!egl->wl_egl_handle) {
        goto err_dlopen_wl_egl;
    }

    WL_EGL_LOAD_FUNC(wl_egl_window_create);
    WL_EGL_LOAD_FUNC(wl_egl_window_destroy);
    WL_EGL_LOAD_FUNC(wl_egl_window_resize);
    WL_EGL_LOAD_FUNC(wl_egl_window_get_attached_size);

    platform->egl = egl;
    return WSI_SUCCESS;

err_dlsym_wl_egl:
    dlclose(egl->wl_egl_handle);
err_dlopen_wl_egl:
err_dlsym_egl:
    free(egl);
    dlclose(egl_handle);
    return WSI_ERROR_PLATFORM;
}

void
wsi_egl_unload(struct wsi_platform *platform)
{
    struct wsi_egl *egl = platform->egl;
    if (!egl) {
        return;
    }

    dlclose(egl->wl_egl_handle);
    dlclose(egl->egl_handle);
    free(egl);
}

WsiResult
wsiGetEglDisplay(
    WsiPlatform platform,
    EGLDisplay *pDisplay)
{
    struct wsi_egl *egl = platform->egl;
    if (!egl) {
        return WSI_ERROR_UNSUPPORTED;
    }

    if (!egl->eglGetPlatformDisplay) {
        egl->eglGetPlatformDisplay =
            (PFNEGLGETPLATFORMDISPLAYPROC)
            egl->eglGetProcAddress("eglGetPlatformDisplay");
    }
    if (!egl->eglGetPlatformDisplay) {
        return WSI_ERROR_PLATFORM;
    }

    EGLAttrib attrib[] = {
        EGL_NONE,
    };

    *pDisplay = egl->eglGetPlatformDisplay(
        EGL_PLATFORM_WAYLAND_KHR,
        platform->wl_display,
        attrib);
    if (*pDisplay == EGL_NO_DISPLAY) {
        return WSI_ERROR_EGL;
    }

    return WSI_SUCCESS;
}

WsiResult
wsiCreateWindowEglSurface(
    WsiWindow window,
    EGLDisplay dpy,
    EGLConfig config,
    EGLSurface *pSurface)
{
    struct wsi_egl *egl = window->platform->egl;
    if (!egl) {
        return WSI_ERROR_UNSUPPORTED;
    }

    if (!egl->eglCreatePlatformWindowSurface) {
        egl->eglCreatePlatformWindowSurface =
            (PFNEGLCREATEPLATFORMWINDOWSURFACEPROC)
            egl->eglGetProcAddress("eglCreatePlatformWindowSurface");
    }
    if (!egl->eglCreatePlatformWindowSurface) {
        return WSI_ERROR_PLATFORM;
    }

    EGLAttrib attrs[] = {
        EGL_NONE,
    };

    *pSurface = egl->eglCreatePlatformWindowSurface(
        dpy,
        config,
        window->wl_egl_window,
        attrs);
    if (*pSurface == EGL_NO_SURFACE) {
        return WSI_ERROR_EGL;
    }

    return WSI_SUCCESS;
}


