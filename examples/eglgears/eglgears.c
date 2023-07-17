/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This is a port of the eglgears demo to libwsi.
 *
 * The file is based on the original eglgears.c file from the Mesa project.
 * https://gitlab.freedesktop.org/mesa/demos/-/blob/main/src/egl/opengl/eglgears.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <wsi/platform.h>
#include <wsi/window.h>
#include <wsi/egl.h>

#include <EGL/egl.h>
#include <GL/gl.h>

const GLfloat TAU_F = 6.28318530717958647692f;

static WsiPlatform   g_platform;
static WsiWindow     g_window;
static WsiExtent     g_extent;

static bool g_running = true;
static bool g_resized = false;

static EGLDisplay  g_display;
static EGLConfig   g_config;
static EGLSurface  g_surface;
static EGLContext  g_context;

static GLfloat g_view_rotx = 20.0f;
static GLfloat g_view_roty = 30.0f;
static GLfloat g_view_rotz = 0.0f;
static GLuint g_gear1;
static GLuint g_gear2;
static GLuint g_gear3;
static GLfloat g_angle = 0.0f;

static const EGLint g_config_attribs[] = {
    EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
    EGL_RED_SIZE,        8,
    EGL_GREEN_SIZE,      8,
    EGL_BLUE_SIZE,       8,
    EGL_ALPHA_SIZE,      8,
    EGL_DEPTH_SIZE,      24,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_NONE
};

static const EGLint g_context_attribs[] = {
    EGL_CONTEXT_MAJOR_VERSION, 2,
    EGL_CONTEXT_MINOR_VERSION, 0,
    EGL_NONE
};

extern int64_t
get_time_ns();

static inline GLfloat
calc_angle(GLint i, GLint teeth) {
    return (GLfloat)i * TAU_F / (GLfloat)teeth;
}

static void
gear(GLfloat inner_radius,
     GLfloat outer_radius,
     GLfloat width,
     GLint teeth,
     GLfloat tooth_depth)
{
    GLfloat r0, r1, r2;
    GLfloat angle, da;
    GLfloat u, v, len;

    r0 = inner_radius;
    r1 = outer_radius - tooth_depth / 2.0f;
    r2 = outer_radius + tooth_depth / 2.0f;

    da = TAU_F / (GLfloat)teeth / 4.0f;

    glShadeModel(GL_FLAT);
    glNormal3f(0.0f, 0.0f, 1.0f);

    glBegin(GL_QUAD_STRIP);
    for (GLint i = 0; i <= teeth; i++) {
        angle = calc_angle(i, teeth);

        glVertex3f(r0 * cosf(angle), r0 * sinf(angle), width * 0.5f);
        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), width * 0.5f);
        if (i < teeth) {
            glVertex3f(r0 * cosf(angle), r0 * sinf(angle), width * 0.5f);
            glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), width * 0.5f);
        }
    }
    glEnd();

    glBegin(GL_QUADS);
    for (GLint i = 0; i < teeth; i++) {
        angle = calc_angle(i, teeth);

        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), width * 0.5f);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), width * 0.5f);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), width * 0.5f);
        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), width * 0.5f);
    }
    glEnd();

    glNormal3f(0.0f, 0.0f, -1.0f);

    glBegin(GL_QUAD_STRIP);
    for (GLint i = 0; i <= teeth; i++) {
        angle = calc_angle(i, teeth);

        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), -width * 0.5f);
        glVertex3f(r0 * cosf(angle), r0 * sinf(angle), -width * 0.5f);
        if (i < teeth) {
            glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), -width * 0.5f);
            glVertex3f(r0 * cosf(angle), r0 * sinf(angle), -width * 0.5f);
        }
    }
    glEnd();

    glBegin(GL_QUADS);
    for (GLint i = 0; i < teeth; i++) {
        angle = calc_angle(i, teeth);

        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), -width * 0.5f);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), -width * 0.5f);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), -width * 0.5f);
        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), -width * 0.5f);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    for (GLint i = 0; i < teeth; i++) {
        angle = calc_angle(i, teeth);

        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), width * 0.5f);
        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), -width * 0.5f);
        u = r2 * cosf(angle + da) - r1 * cosf(angle);
        v = r2 * sinf(angle + da) - r1 * sinf(angle);
        len = sqrtf(u * u + v * v);
        u /= len;
        v /= len;
        glNormal3f(v, -u, 0.0f);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), width * 0.5f);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), -width * 0.5f);
        glNormal3f(cosf(angle), sinf(angle), 0.0f);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), width * 0.5f);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), -width * 0.5f);
        u = r1 * cosf(angle + 3.0f * da) - r2 * cosf(angle + 2.0f * da);
        v = r1 * sinf(angle + 3.0f * da) - r2 * sinf(angle + 2.0f * da);
        glNormal3f(v, -u, 0.0f);
        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), width * 0.5f);
        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), -width * 0.5f);
        glNormal3f(cosf(angle), sinf(angle), 0.0f);
    }

    glVertex3f(r1 * cosf(0), r1 * sinf(0), width * 0.5f);
    glVertex3f(r1 * cosf(0), r1 * sinf(0), -width * 0.5f);

    glEnd();

    glShadeModel(GL_SMOOTH);

    glBegin(GL_QUAD_STRIP);
    for (GLint i = 0; i <= teeth; i++) {
        angle = calc_angle(i, teeth);
        glNormal3f(-cosf(angle), -sinf(angle), 0.0f);
        glVertex3f(r0 * cosf(angle), r0 * sinf(angle), -width * 0.5f);
        glVertex3f(r0 * cosf(angle), r0 * sinf(angle), width * 0.5f);
    }
    glEnd();
}

static void
draw()
{
    if (g_resized) {
        glViewport(0, 0, (GLsizei) g_extent.width, (GLsizei) g_extent.height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        GLfloat hf = (GLfloat) g_extent.height;
        GLfloat wf = (GLfloat) g_extent.width;

        if (hf > wf) {
            GLfloat aspect = hf / wf;
            glFrustum(-1.0, 1.0, -aspect, aspect, 5.0, 60.0);
        } else {
            GLfloat aspect = wf / hf;
            glFrustum(-aspect, aspect, -1.0, 1.0, 5.0, 60.0);
        }

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -40.0f);
        g_resized = false;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.8f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glRotatef(g_view_rotx, 1.0f, 0.0f, 0.0f);
    glRotatef(g_view_roty, 0.0f, 1.0f, 0.0f);
    glRotatef(g_view_rotz, 0.0f, 0.0f, 1.0f);

    glPushMatrix();
    glTranslatef(-3.0f, -2.0f, 0.0f);
    glRotatef(g_angle, 0.0f, 0.0f, 1.0f);
    glCallList(g_gear1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(3.1f, -2.0f, 0.0f);
    glRotatef(-2.0f * g_angle - 9.0f, 0.0f, 0.0f, 1.0f);
    glCallList(g_gear2);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-3.1f, 4.2f, 0.0f);
    glRotatef(-2.0f * g_angle - 25.0f, 0.0f, 0.0f, 1.0f);
    glCallList(g_gear3);
    glPopMatrix();

    glPopMatrix();
}

static void
create_gears()
{
    static GLfloat pos[4] = { 5.0f, 5.0f, 10.0f, 0.0f };
    static GLfloat red[4] = { 0.8f, 0.1f, 0.0f, 1.0f };
    static GLfloat green[4] = { 0.0f, 0.8f, 0.2f, 1.0f };
    static GLfloat blue[4] = { 0.2f, 0.2f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);

    g_gear1 = glGenLists(1);
    glNewList(g_gear1, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
    gear(1.0f, 4.0f, 1.0f, 20, 0.7f);
    glEndList();

    g_gear2 = glGenLists(1);
    glNewList(g_gear2, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
    gear(0.5f, 2.0f, 2.0f, 10, 0.7f);
    glEndList();

    g_gear3 = glGenLists(1);
    glNewList(g_gear3, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
    gear(1.3f, 2.0f, 0.5f, 10, 0.7f);
    glEndList();

    glEnable(GL_NORMALIZE);
}

static void
close_window(void *pUserData, const WsiCloseWindowEvent *pInfo)
{
    g_running = false;
}

static void
configure_window(void *pUserData, const WsiConfigureWindowEvent *pInfo)
{
    g_extent = pInfo->extent;
    g_resized = true;
}

int
main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;

    WsiPlatformCreateInfo platform_info = {
        .sType = WSI_STRUCTURE_TYPE_PLATFORM_CREATE_INFO,
        .pNext = NULL,
    };

    WsiResult res = wsiCreatePlatform(&platform_info, &g_platform);
    if (res != WSI_SUCCESS) {
        fprintf(stderr, "wsiCreatePlatform failed: %d\n", res);
        goto err_wsi_platform;
    }

    res = wsiGetEGLDisplay(g_platform, &g_display);
    if (res != WSI_SUCCESS) {
        fprintf(stderr, "wsiGetEGLDisplay failed: %d", res);
        if (res == WSI_ERROR_EGL) {
            fprintf(stderr, " 0x%08x", eglGetError());
        }
        fprintf(stderr, "\n");
        goto err_wsi_display;
    }

    EGLint major, minor;
    EGLBoolean ok = eglInitialize(g_display, &major, &minor);
    if (ok == EGL_FALSE) {
        fprintf(stderr, "eglInitialize failed: 0x%08x\n", eglGetError());
        goto err_egl_init;
    }

    if (major < 1 || (major == 1 && minor < 4)) {
        fprintf(stderr, "EGL version %d.%d is too old\n", major, minor);
        goto err_egl_version;
    }

    ok = eglBindAPI(EGL_OPENGL_API);
    if (ok == EGL_FALSE) {
        fprintf(stderr, "eglBindAPI failed: 0x%08x\n", eglGetError());
        goto err_egl_bind;
    }

    EGLint num_configs = 1;
    ok = eglChooseConfig(g_display, g_config_attribs, &g_config, 1, &num_configs);
    if (ok == EGL_FALSE) {
        fprintf(stderr, "eglChooseConfig failed: 0x%08x\n", eglGetError());
        goto err_egl_config;
    }

    if (num_configs == 0) {
        fprintf(stderr, "eglChooseConfig failed: no configs found\n");
        goto err_egl_config;
    }

    g_context = eglCreateContext(
        g_display,
        g_config,
        EGL_NO_CONTEXT,
        g_context_attribs);
    if (g_context == EGL_NO_CONTEXT) {
        fprintf(stderr, "eglCreateContext failed: 0x%08x\n", eglGetError());
        goto err_egl_context;
    }

    WsiWindowCreateInfo info = {
        .sType = WSI_STRUCTURE_TYPE_WINDOW_CREATE_INFO,
        .pNext = NULL,
        .extent.width = 300,
        .extent.height = 300,
        .pTitle = "Gears",
        .pUserData = NULL,
        .pfnCloseWindow = close_window,
        .pfnConfigureWindow = configure_window,
    };

    res = wsiCreateWindow(g_platform, &info, &g_window);
    if (res != WSI_SUCCESS) {
        fprintf(stderr, "wsiCreateWindow failed: %d\n", res);
        goto err_wsi_window;
    }

    while (true) {
        res = wsiDispatchEvents(g_platform, -1);
        if (res != WSI_SUCCESS || g_resized) {
            break;
        }
    }
    if (res != WSI_SUCCESS) {
        goto err_wsi_dispatch;
    }

    res = wsiCreateWindowEGLSurface(g_window, g_display, g_config, &g_surface);
    if (res != WSI_SUCCESS) {
        fprintf(stderr, "wsiCreateWindowEGLSurface failed: %d", res);
        if (res == WSI_ERROR_EGL) {
            fprintf(stderr, " 0x%08x", eglGetError());
        }
        fprintf(stderr, "\n");
        goto err_wsi_surface;
    }

    ok = eglMakeCurrent(g_display, g_surface, g_surface, g_context);
    if (ok == EGL_FALSE) {
        fprintf(stderr, "eglMakeCurrent failed: 0x%08x\n", eglGetError());
        goto err_egl_current;
    }

    ok = eglSwapInterval(g_display, 0);
    if (ok == EGL_FALSE) {
        fprintf(stderr, "eglSwapInterval failed: 0x%08x\n", eglGetError());
        goto err_egl_interval;
    }

    create_gears();

    int64_t last_time = get_time_ns();

    while(true) {
        draw();

        ok = eglSwapBuffers(g_display, g_surface);
        if (ok == EGL_FALSE) {
            printf("eglSwapBuffers failed: %d\n", eglGetError());
            break;
        }

        int64_t now = get_time_ns();
        int64_t dt = now - last_time;
        last_time = now;

        float time = (float)dt / 1000000000.0f;
        g_angle += time * 70.0f;
        g_angle = fmodf(g_angle, 360.0f);

        res = wsiDispatchEvents(g_platform, 0);
        if (res != WSI_SUCCESS || !g_running) {
            break;
        }
    }

    ret = EXIT_SUCCESS;
err_egl_interval:
err_egl_current:
    wsiDestroyWindowEGLSurface(g_window, g_display, g_surface);
err_wsi_surface:
err_wsi_dispatch:
    wsiDestroyWindow(g_window);
err_wsi_window:
    eglDestroyContext(g_display, g_context);
err_egl_context:
err_egl_config:
err_egl_bind:
err_egl_version:
    eglTerminate(g_display);
err_egl_init:
err_wsi_display:
    wsiDestroyPlatform(g_platform);
err_wsi_platform:
    return ret;
}
