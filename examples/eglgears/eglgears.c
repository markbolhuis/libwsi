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

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <wsi/platform.h>
#include <wsi/window.h>
#include <wsi/egl/egl.h>

#include <EGL/egl.h>
#include <GL/gl.h>

#ifndef M_PI
#define M_PI 3.14159265
#endif

static WsiPlatform   g_platform;
static WsiEventQueue g_event_queue;
static WsiWindow     g_window;
static WsiExtent     g_extent = { 300, 300 };

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

static void
gear(GLfloat inner_radius,
     GLfloat outer_radius,
     GLfloat width,
     GLint teeth,
     GLfloat tooth_depth)
{
    GLint i;
    GLfloat r0, r1, r2;
    GLfloat angle, da;
    GLfloat u, v, len;

    r0 = inner_radius;
    r1 = outer_radius - tooth_depth / 2.0;
    r2 = outer_radius + tooth_depth / 2.0;

    da = 2.0 * M_PI / teeth / 4.0;

    glShadeModel(GL_FLAT);

    glNormal3f(0.0, 0.0, 1.0);

    /* draw front face */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++) {
        angle = i * 2.0 * M_PI / teeth;
        glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
        glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
        if (i < teeth) {
            glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
            glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
                       width * 0.5);
        }
    }
    glEnd();

    /* draw front sides of teeth */
    glBegin(GL_QUADS);
    da = 2.0 * M_PI / teeth / 4.0;
    for (i = 0; i < teeth; i++) {
        angle = i * 2.0 * M_PI / teeth;

        glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
        glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
        glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
                   width * 0.5);
        glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
                   width * 0.5);
    }
    glEnd();

    glNormal3f(0.0, 0.0, -1.0);

    /* draw back face */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++) {
        angle = i * 2.0 * M_PI / teeth;
        glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
        glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
        if (i < teeth) {
            glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
                       -width * 0.5);
            glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
        }
    }
    glEnd();

    /* draw back sides of teeth */
    glBegin(GL_QUADS);
    da = 2.0 * M_PI / teeth / 4.0;
    for (i = 0; i < teeth; i++) {
        angle = i * 2.0 * M_PI / teeth;

        glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
                   -width * 0.5);
        glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
                   -width * 0.5);
        glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
        glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
    }
    glEnd();

    /* draw outward faces of teeth */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i < teeth; i++) {
        angle = i * 2.0 * M_PI / teeth;

        glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
        glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
        u = r2 * cos(angle + da) - r1 * cos(angle);
        v = r2 * sin(angle + da) - r1 * sin(angle);
        len = sqrt(u * u + v * v);
        u /= len;
        v /= len;
        glNormal3f(v, -u, 0.0);
        glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
        glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
        glNormal3f(cos(angle), sin(angle), 0.0);
        glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
                   width * 0.5);
        glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
                   -width * 0.5);
        u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
        v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
        glNormal3f(v, -u, 0.0);
        glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
                   width * 0.5);
        glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
                   -width * 0.5);
        glNormal3f(cos(angle), sin(angle), 0.0);
    }

    glVertex3f(r1 * cos(0), r1 * sin(0), width * 0.5);
    glVertex3f(r1 * cos(0), r1 * sin(0), -width * 0.5);

    glEnd();

    glShadeModel(GL_SMOOTH);

    /* draw inside radius cylinder */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++) {
        angle = i * 2.0 * M_PI / teeth;
        glNormal3f(-cos(angle), -sin(angle), 0.0);
        glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
        glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
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

    EGLBoolean ok = eglSwapBuffers(g_display, g_surface);
    if (ok == EGL_FALSE) {
        printf("eglSwapBuffers failed: %d\n", eglGetError());
    }
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

    /* make the gears */
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
handle_window_close(void *data)
{
    g_running = false;
}

static void
handle_window_configure(void *data, WsiExtent extent)
{
    g_extent = extent;
    g_resized = true;
}

int
main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;

    WsiResult res = wsiCreatePlatform(&g_platform);
    if (res != WSI_SUCCESS) {
        fprintf(stderr, "wsiCreatePlatform failed: %d\n", res);
        goto err_wsi_platform;
    }

    g_event_queue = wsiGetDefaultEventQueue(g_platform);

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

    WsiWindowCreateInfo info = {0};
    info.eventQueue = g_event_queue;
    info.extent = g_extent;
    info.pTitle = "Gears";
    info.pfnClose = handle_window_close;
    info.pfnConfigure = handle_window_configure;

    res = wsiCreateWindow(g_platform, &info, &g_window);
    if (res != WSI_SUCCESS) {
        fprintf(stderr, "wsiCreateWindow failed: %d\n", res);
        goto err_wsi_window;
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

    struct timespec epoch;
    clock_gettime(CLOCK_MONOTONIC, &epoch);
    float epoch_d = (float)epoch.tv_sec + (float)epoch.tv_nsec / 1000000000.0f;

    WsiExtent extent;
    while(true) {
        res = wsiPollEventQueue(g_event_queue);
        if (res != WSI_SUCCESS || !g_running) {
            break;
        }

        draw();

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        float now_d = (float)now.tv_sec + (float)now.tv_nsec / 1000000000.0f;

        g_angle = fmodf(70.0f * (now_d - epoch_d), 360.0f);
    }

    ret = EXIT_SUCCESS;
err_egl_interval:
err_egl_current:
    wsiDestroyWindowEGLSurface(g_window, g_display, g_surface);
err_wsi_surface:
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
