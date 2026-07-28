// input.h
#pragma once
#include <stdbool.h>

#define INPUT_KEY_COUNT 256

typedef enum MouseButton {
    MOUSE_LEFT = 0,
    MOUSE_RIGHT = 1,
    MOUSE_MIDDLE = 2
} MouseButton;

typedef struct {
    int x, y;
    int delta_x, delta_y;
    bool left;
    bool right;
    bool middle;
    int wheel;
} MouseData;

typedef struct {
    struct {
        bool previous[INPUT_KEY_COUNT];
        bool current[INPUT_KEY_COUNT];
    } keyboard;
    
    struct {
        MouseData previous;
        MouseData current;
    } mouse;
} Input;

// Funciones existentes...
void input_init(Input* input);
void input_begin_frame(Input* input);
void input_on_key_down(Input* input, int key);
void input_on_key_up(Input* input, int key);

// Mouse - posición absoluta
void input_on_mouse_move(Input* input, int x, int y);

// Mouse - movimiento relativo (NUEVO)
void input_on_mouse_delta(Input* input, int delta_x, int delta_y);

void input_on_mouse_button_down(Input* input, MouseButton button);
void input_on_mouse_button_up(Input* input, MouseButton button);
void input_on_mouse_wheel(Input* input, int delta);

// Consultas
bool input_is_down(Input* input, int key);
bool input_went_down(Input* input, int key);
bool input_went_up(Input* input, int key);

bool input_mouse_is_down(Input* input, MouseButton button);
bool input_mouse_went_down(Input* input, MouseButton button);
bool input_mouse_went_up(Input* input, MouseButton button);

// Obtener posición (modo libre)
void input_get_mouse_position(Input* input, int* x, int* y);

// Obtener delta (modo capturado) - NUEVO
void input_get_mouse_delta(Input* input, int* delta_x, int* delta_y);