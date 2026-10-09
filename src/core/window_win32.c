// window_win32.c
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "window.h"

static HINSTANCE g_instance = NULL;
static const wchar_t* g_class_name = L"EngineWindowClass";

// Forward declaration: window_init la usa (wc.lpfnWndProc),
// pero su definición está más abajo en el archivo.
static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

// Convierte un RECT de Win32 a nuestro WindowRect portable.
static void rect_to_window_rect(const RECT* r, WindowRect* out) {
    out->left   = r->left;
    out->top    = r->top;
    out->right  = r->right;
    out->bottom = r->bottom;
}

static void window_set_fullscreen(Window* window) {
    if (window->is_fullscreen) return;

    HWND hwnd = (HWND)window->handle;

    // Guardar posición y tamaño actuales
    RECT r;
    GetWindowRect(hwnd, &r);
    rect_to_window_rect(&r, &window->windowed_rect);

    // Obtener dimensiones del monitor
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitor_info = {0};
    monitor_info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &monitor_info);

    // Cambiar a pantalla completa
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    SetWindowPos(hwnd, HWND_TOP,
                 monitor_info.rcMonitor.left,
                 monitor_info.rcMonitor.top,
                 monitor_info.rcMonitor.right  - monitor_info.rcMonitor.left,
                 monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    window->is_fullscreen = true;
}

static void window_set_small_screen(Window* window) {
    HWND hwnd = (HWND)window->handle;

    // Volver a modo ventana
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
    SetWindowPos(hwnd, HWND_NOTOPMOST,
                 window->windowed_rect.left,
                 window->windowed_rect.top,
                 window->windowed_rect.right  - window->windowed_rect.left,
                 window->windowed_rect.bottom - window->windowed_rect.top,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    window->is_fullscreen = false;
}

static void window_center_cursor(Window* window) {
    HWND hwnd = (HWND)window->handle;
    RECT rect;
    GetClientRect(hwnd, &rect);

    POINT center = {
        (rect.right  - rect.left) / 2,
        (rect.bottom - rect.top)  / 2
    };
    ClientToScreen(hwnd, &center);

    SetCursorPos(center.x, center.y);
}

static void window_toggle_fullscreen(Window* window) {
    if (window->is_fullscreen) {
        window_set_small_screen(window);
    } else {
        window_set_fullscreen(window);
    }

    if (window->mouse_mode == MOUSE_MODE_CAPTURED) {
        window_center_cursor(window);
    }
}


// -----------------------------------------------------------------------------
// Mouse mode
// -----------------------------------------------------------------------------

void window_set_mouse_mode(Window* window, MouseMode mode) {
    if (window->mouse_mode == mode) return;
    HWND hwnd = (HWND)window->handle;

    // Entrar en nuevo modo
    window->mouse_mode   = mode;
    window->changed_mode = true;

    switch (mode) {
        case MOUSE_MODE_FREE:
            window->cursor_visible = true;
            input_on_mouse_delta(window->input, 0, 0);
            ReleaseCapture();
            ShowCursor(TRUE);
            break;

        case MOUSE_MODE_CAPTURED:
            window->cursor_visible = false;
            ShowCursor(FALSE);
            SetCapture(hwnd);
            window_center_cursor(window);
            break;
    }
}

MouseMode window_get_mouse_mode(Window* window) {
    return window->mouse_mode;
}

void window_toggle_mouse_mode(Window* window) {
    if (window->mouse_mode == MOUSE_MODE_FREE) {
        window_set_mouse_mode(window, MOUSE_MODE_CAPTURED);
    } else {
        window_set_mouse_mode(window, MOUSE_MODE_FREE);
    }
}

bool window_is_cursor_visible(Window* window) {
    return window->cursor_visible;
}


// -----------------------------------------------------------------------------
// Window procedure
// -----------------------------------------------------------------------------

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!window) {
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    switch (msg) {
        case WM_CLOSE:
            window->should_close = true;
            return 0;

        case WM_SIZE:
            window->is_minimized = (wparam == SIZE_MINIMIZED);
            window->width  = LOWORD(lparam);
            window->height = HIWORD(lparam);
            window->resized = true;
            return 0;

        case WM_KEYDOWN:
            input_on_key_down(window->input, (int)wparam);

            if (wparam == VK_F11) {
                window_toggle_fullscreen(window);
            }
            if (wparam == VK_TAB) {
                window_toggle_mouse_mode(window);
            }
            if (wparam == VK_ESCAPE) {
                window_set_mouse_mode(window, MOUSE_MODE_FREE);
            }
            return 0;

        case WM_KEYUP:
            input_on_key_up(window->input, (int)wparam);
            return 0;

        case WM_LBUTTONDOWN:
            input_on_mouse_button_down(window->input, 0);
            return 0;

        case WM_LBUTTONUP:
            input_on_mouse_button_up(window->input, 0);
            return 0;

        case WM_RBUTTONDOWN:
            input_on_mouse_button_down(window->input, 1);
            return 0;

        case WM_RBUTTONUP:
            input_on_mouse_button_up(window->input, 1);
            return 0;

        case WM_MBUTTONDOWN:
            input_on_mouse_button_down(window->input, 2);
            return 0;

        case WM_MBUTTONUP:
            input_on_mouse_button_up(window->input, 2);
            return 0;

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
            input_on_mouse_wheel(window->input, delta);
            return 0;
        }

        case WM_MOUSEMOVE: {
            if (window->resized || window->changed_mode) {
                return 0;
            }

            int x = GET_X_LPARAM(lparam);
            int y = GET_Y_LPARAM(lparam);

            if (window->mouse_mode == MOUSE_MODE_FREE) {
                // Modo libre: reportar posición absoluta
                input_on_mouse_move(window->input, x, window->height - 1 - y);
            } else {
                // Modo capturado: calcular delta desde el centro
                RECT rect;
                GetClientRect(hwnd, &rect);
                int center_x = (rect.right  - rect.left) / 2;
                int center_y = (rect.bottom - rect.top)  / 2;

                int delta_x = x - center_x;
                int delta_y = y - center_y;

                input_on_mouse_delta(window->input, delta_x, delta_y);

                if (delta_x != 0 || delta_y != 0) {
                    window_center_cursor(window);
                }
            }
            return 0;
        }

        case WM_KILLFOCUS:
            // Al perder foco, forzar modo libre
            window_set_mouse_mode(window, MOUSE_MODE_FREE);
            return 0;

        default:
            return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}


// -----------------------------------------------------------------------------
// Init / poll / destroy
// -----------------------------------------------------------------------------

bool window_init(Window* window, uint32_t width, uint32_t height,
                 const char* title, Input* input) {
    if (!window) return false;

    g_instance = GetModuleHandle(NULL);

    /* Registrar la clase de ventana (una sola vez) */
    static bool class_registered = false;
    if (!class_registered) {
        WNDCLASSEXW wc = {0};
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = window_proc;
        wc.hInstance     = g_instance;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = g_class_name;

        if (!RegisterClassExW(&wc)) {
            return false;
        }

        class_registered = true;
    }

    /* Convertir título a wide char */
    wchar_t title_w[256];
    MultiByteToWideChar(CP_UTF8, 0, title, -1, title_w, 256);

    /* Crear la ventana */
    HWND hwnd = CreateWindowExW(0, g_class_name, title_w, WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                (int)width, (int)height,
                                NULL, NULL, g_instance, NULL);

    if (!hwnd) {
        return false;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    window->input         = input;
    window->handle        = hwnd;
    window->instance      = g_instance;
    window->should_close  = false;
    window->resized       = false;
    window->changed_mode  = false;
    window->is_minimized  = false;
    window->is_fullscreen = false;

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);

    // Obtener el tamaño real de la ventana (cliente)
    RECT client_rect;
    GetClientRect(hwnd, &client_rect);
    window->width  = client_rect.right - client_rect.left;
    window->height = client_rect.bottom - client_rect.top;

    window_set_mouse_mode(window, MOUSE_MODE_CAPTURED);
    return true;
}

void window_poll_events(Window* window) {
    window->resized      = false;
    window->changed_mode = false;

    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void window_destroy(Window* window) {
    if (!window || !window->handle) return;

    DestroyWindow((HWND)window->handle);
    window->handle = NULL;
}

bool vk_platform_is_kosmickrisp(void) {
    return false;   // KosmicKrisp solo existe en macOS.
}