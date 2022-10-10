#ifndef WSI_SRC_WAYLAND_POINTER_PRIVATE_H
#define WSI_SRC_WAYLAND_POINTER_PRIVATE_H

enum wsi_pointer_event {
    WSI_POINTER_EVENT_NONE = 0,
    WSI_POINTER_EVENT_ENTER = 1,
    WSI_POINTER_EVENT_LEAVE = 2,
    WSI_POINTER_EVENT_MOTION = 4,
    WSI_POINTER_EVENT_BUTTON = 8,
    WSI_POINTER_EVENT_AXIS = 16,
    WSI_POINTER_EVENT_AXIS_SOURCE = 32,
    WSI_POINTER_EVENT_AXIS_STOP = 64,
    WSI_POINTER_EVENT_AXIS_DISCRETE = 128,
};

struct wsi_pointer_frame {
    enum wsi_pointer_event mask;

    uint32_t serial;
    uint32_t time;

    struct wl_surface *enter;
    struct wl_surface *leave;

    uint32_t button;
    uint32_t state;

    double x;
    double y;

    uint32_t axis_source;
    struct {
        double value;
        int32_t discrete;
        uint32_t start_time;
        uint32_t stop_time;
    } axes[2];
};

struct wsi_pointer {
    struct wsi_seat        *seat;

    struct wl_pointer      *wl_pointer;

    struct wl_cursor_theme *wl_cursor_theme;
    struct wl_cursor       *wl_cursor;
    struct wl_surface      *wl_cursor_surface;

    struct wsi_pointer_frame frame;
};

bool
wsi_pointer_create(struct wsi_seat *seat);

void
wsi_pointer_destroy(struct wsi_seat *seat);

#endif
