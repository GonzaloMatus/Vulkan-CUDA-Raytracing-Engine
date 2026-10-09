// window.h
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "input.h"

typedef enum {
    MOUSE_MODE_FREE,
    MOUSE_MODE_CAPTURED
} MouseMode;

// Rect portable. En Win32 se llena desde RECT; en Cocoa desde NSRect.
typedef struct {
    int32_t left, top, right, bottom;
} WindowRect;

typedef struct {
    Input* input;
    void*  handle;    // HWND (Win32) o NSWindow* (Cocoa). Opaco.
    void*  instance;  // HINSTANCE (Win32) o NSApplication* (Cocoa). Opaco.
    uint32_t width;
    uint32_t height;
    bool changed_mode;
    bool should_close;
    bool resized;
    bool is_minimized;
    bool is_fullscreen;
    WindowRect windowed_rect;

    MouseMode mouse_mode;
    bool cursor_visible;
} Window;

bool window_init(Window* window, uint32_t width, uint32_t height,
                 const char* title, Input* input);
void window_poll_events(Window* window);
void window_destroy(Window* window);

void window_set_mouse_mode(Window* window, MouseMode mode);
MouseMode window_get_mouse_mode(Window* window);
void window_toggle_mouse_mode(Window* window);
bool window_is_cursor_visible(Window* window);