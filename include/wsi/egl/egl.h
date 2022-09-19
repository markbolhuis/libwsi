#ifndef WSI_INCLUDE_EGL_EGL_H
#define WSI_INCLUDE_EGL_EGL_H

#include "../common.h"

typedef void *EGLDisplay;
typedef void *EGLConfig;
typedef void *EGLSurface;

WsiResult
wsiGetEglDisplay(
    WsiPlatform platform,
    EGLDisplay *pDisplay);

WsiResult
wsiCreateWindowEglSurface(
    WsiWindow window,
    EGLDisplay dpy,
    EGLConfig config,
    EGLSurface *pSurface);

#endif
