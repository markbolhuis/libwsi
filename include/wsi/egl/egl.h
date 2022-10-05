#ifndef WSI_INCLUDE_EGL_EGL_H
#define WSI_INCLUDE_EGL_EGL_H

#include "../common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *EGLDisplay;
typedef void *EGLConfig;
typedef void *EGLSurface;

WsiResult
wsiGetEGLDisplay(
    WsiPlatform platform,
    EGLDisplay *pDisplay);

WsiResult
wsiCreateWindowEGLSurface(
    WsiWindow window,
    EGLDisplay dpy,
    EGLConfig config,
    EGLSurface *pSurface);

#ifdef __cplusplus
}
#endif

#endif
