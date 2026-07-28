// window.h
#pragma once
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include "input.h"

typedef enum {
    MOUSE_MODE_FREE,      // Cursor visible, movimiento absoluto
    MOUSE_MODE_CAPTURED   // Cursor invisible, movimiento relativo
} MouseMode;

typedef struct {
    Input* input;          // Referencia al sistema de input
    void* handle;          // HWND
    uint32_t width;
    uint32_t height;
    bool changed_mode;
    bool should_close;
    bool resized;
    bool is_minimized;
    bool is_fullscreen;
    RECT windowed_rect;
    
    // Solo estado del mouse para la ventana
    MouseMode mouse_mode;
    bool cursor_visible;
} Window;

// Funciones de ventana
bool window_init(Window* window, uint32_t width, uint32_t height, 
                 const char* title, Input* input);
void window_poll_events(Window* window);
void window_destroy(Window* window);

// Control del mouse
void window_set_mouse_mode(Window* window, MouseMode mode);
MouseMode window_get_mouse_mode(Window* window);
void window_toggle_mouse_mode(Window* window);
bool window_is_cursor_visible(Window* window);