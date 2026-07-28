// input.c
#include "input.h"
#include <string.h>
#include <stdio.h>
void input_init(Input* input) {
    memset(input, 0, sizeof(Input));
}

void input_begin_frame(Input* input) {
    // Copiar estados anteriores
    memcpy(input->keyboard.previous, input->keyboard.current, sizeof(input->keyboard.current));
    
    input->mouse.previous = input->mouse.current;
    input->mouse.current.delta_x = 0;
    input->mouse.current.delta_y = 0;
    input->mouse.current.left = 0;
    input->mouse.current.right = 0;
    input->mouse.current.middle = 0;
    input->mouse.current.wheel = 0;
}

void input_on_mouse_move(Input* input, int x, int y) {
    input->mouse.current.x = x;
    input->mouse.current.y = y;
}

// NUEVA FUNCIÓN: Para movimiento relativo (modo FPS)
void input_on_mouse_delta(Input* input, int delta_x, int delta_y) {
    input->mouse.current.delta_x = delta_x;
    input->mouse.current.delta_y = delta_y;
}

// Funciones existentes...
void input_on_key_down(Input* input, int key) {
    if (key >= 0 && key < INPUT_KEY_COUNT) {
        input->keyboard.current[key] = true;
    }
}

void input_on_key_up(Input* input, int key) {
    if (key >= 0 && key < INPUT_KEY_COUNT) {
        input->keyboard.current[key] = false;
    }
}

void input_on_mouse_button_down(Input* input, MouseButton button) {
    switch (button) {
        case MOUSE_LEFT: input->mouse.current.left = true; break;
        case MOUSE_RIGHT: input->mouse.current.right = true; break;
        case MOUSE_MIDDLE: input->mouse.current.middle = true; break;
    }
}

void input_on_mouse_button_up(Input* input, MouseButton button) {
    switch (button) {
        case MOUSE_LEFT: input->mouse.current.left = false; break;
        case MOUSE_RIGHT: input->mouse.current.right = false; break;
        case MOUSE_MIDDLE: input->mouse.current.middle = false; break;
    }
}

void input_on_mouse_wheel(Input* input, int delta) {
    input->mouse.current.wheel = delta;
}

// Funciones de consulta existentes...
bool input_is_down(Input* input, int key) {
    return input->keyboard.current[key];
}

bool input_went_down(Input* input, int key) {
    return input->keyboard.current[key] && !input->keyboard.previous[key];
}

bool input_went_up(Input* input, int key) {
    return !input->keyboard.current[key] && input->keyboard.previous[key];
}

bool input_mouse_is_down(Input* input, MouseButton button) {
    switch (button) {
        case MOUSE_LEFT: return input->mouse.current.left;
        case MOUSE_RIGHT: return input->mouse.current.right;
        case MOUSE_MIDDLE: return input->mouse.current.middle;
        default: return false;
    }
}

bool input_mouse_went_down(Input* input, MouseButton button) {
    switch (button) {
        case MOUSE_LEFT: return input->mouse.current.left && !input->mouse.previous.left;
        case MOUSE_RIGHT: return input->mouse.current.right && !input->mouse.previous.right;
        case MOUSE_MIDDLE: return input->mouse.current.middle && !input->mouse.previous.middle;
        default: return false;
    }
}

bool input_mouse_went_up(Input* input, MouseButton button) {
    switch (button) {
        case MOUSE_LEFT: return !input->mouse.current.left && input->mouse.previous.left;
        case MOUSE_RIGHT: return !input->mouse.current.right && input->mouse.previous.right;
        case MOUSE_MIDDLE: return !input->mouse.current.middle && input->mouse.previous.middle;
        default: return false;
    }
}

void input_get_mouse_position(Input* input, int* x, int* y) {
    if (x) *x = input->mouse.current.x;
    if (y) *y = input->mouse.current.y;
}

void input_get_mouse_delta(Input* input, int* delta_x, int* delta_y) {
    if (delta_x) *delta_x = input->mouse.current.delta_x;
    if (delta_y) *delta_y = input->mouse.current.delta_y;
}

int input_get_mouse_wheel(Input* input) {
    int wheel = input->mouse.current.wheel;
    return wheel;
}