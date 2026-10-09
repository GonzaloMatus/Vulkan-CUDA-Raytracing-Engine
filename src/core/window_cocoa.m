// window_cocoa.m
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <stdlib.h>
#include "window.h"

// -----------------------------------------------------------------------------
// Vista respaldada por CAMetalLayer
// -----------------------------------------------------------------------------
// NSView por defecto no expone un CAMetalLayer. Hay que subclasear
// y devolverlo desde makeBackingLayer.

@interface EngineView : NSView
@end

@implementation EngineView

- (CALayer*)makeBackingLayer {
    return [CAMetalLayer layer];
}

- (BOOL)wantsUpdateLayer {
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)isOpaque {
    return YES;
}

@end


// -----------------------------------------------------------------------------
// Estado nativo
// -----------------------------------------------------------------------------

typedef struct {
    NSWindow* ns_window;
    EngineView* ns_view;   // subclase, no NSView genérico
    id        key_monitor;
    id        mouse_monitor;
} CocoaState;

static CocoaState* get_state(Window* w) {
    return (CocoaState*)w->handle;
}

// Expuesta para vk_platform_cocoa.m
void* window_cocoa_get_metal_layer(const Window* w) {
    CocoaState* st = (CocoaState*)w->handle;
    return (__bridge void*)st->ns_view.layer;
}


// -----------------------------------------------------------------------------
// Fullscreen
// -----------------------------------------------------------------------------

static void window_set_fullscreen(Window* window) {
    if (window->is_fullscreen) return;
    CocoaState* st = get_state(window);
    NSWindow* win = st->ns_window;

    NSRect frame = win.frame;
    window->windowed_rect.left   = (int32_t)frame.origin.x;
    window->windowed_rect.top    = (int32_t)frame.origin.y;
    window->windowed_rect.right  = (int32_t)(frame.origin.x + frame.size.width);
    window->windowed_rect.bottom = (int32_t)(frame.origin.y + frame.size.height);

    [win setStyleMask:NSWindowStyleMaskBorderless];
    NSScreen* screen = [win screen] ?: [NSScreen mainScreen];
    [win setFrame:screen.frame display:YES];
    [win setLevel:NSMainMenuWindowLevel + 1];
    window->is_fullscreen = true;
}

static void window_set_small_screen(Window* window) {
    CocoaState* st = get_state(window);
    NSWindow* win = st->ns_window;
    [win setStyleMask:(NSWindowStyleMaskTitled |
                       NSWindowStyleMaskClosable |
                       NSWindowStyleMaskMiniaturizable |
                       NSWindowStyleMaskResizable)];
    NSRect r = NSMakeRect(window->windowed_rect.left,
                          window->windowed_rect.top,
                          window->windowed_rect.right  - window->windowed_rect.left,
                          window->windowed_rect.bottom - window->windowed_rect.top);
    [win setFrame:r display:YES];
    [win setLevel:NSNormalWindowLevel];
    window->is_fullscreen = false;
}

static void window_toggle_fullscreen(Window* window) {
    if (window->is_fullscreen) window_set_small_screen(window);
    else                       window_set_fullscreen(window);
}


// -----------------------------------------------------------------------------
// Mouse
// -----------------------------------------------------------------------------

void window_set_mouse_mode(Window* window, MouseMode mode) {
    if (window->mouse_mode == mode) return;
    window->mouse_mode   = mode;
    window->changed_mode = true;

    switch (mode) {
        case MOUSE_MODE_FREE:
            window->cursor_visible = true;
            input_on_mouse_delta(window->input, 0, 0);
            CGAssociateMouseAndMouseCursorPosition(true);
            [NSCursor unhide];
            break;

        case MOUSE_MODE_CAPTURED:
            window->cursor_visible = false;
            CGAssociateMouseAndMouseCursorPosition(false);
            [NSCursor hide];
            break;
    }
}

MouseMode window_get_mouse_mode(Window* window) { return window->mouse_mode; }

void window_toggle_mouse_mode(Window* window) {
    window_set_mouse_mode(window,
        window->mouse_mode == MOUSE_MODE_FREE ? MOUSE_MODE_CAPTURED : MOUSE_MODE_FREE);
}

bool window_is_cursor_visible(Window* window) { return window->cursor_visible; }


// -----------------------------------------------------------------------------
// Init / destroy
// -----------------------------------------------------------------------------

bool window_init(Window* window, uint32_t width, uint32_t height,
                 const char* title, Input* input) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp finishLaunching];

        NSRect frame = NSMakeRect(0, 0, width, height);
        NSWindowStyleMask style =
            NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
            NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

        NSWindow* ns_window =
            [[NSWindow alloc] initWithContentRect:frame
                                        styleMask:style
                                          backing:NSBackingStoreBuffered
                                            defer:NO];
        [ns_window setTitle:[NSString stringWithUTF8String:title]];

        // Vista respaldada por CAMetalLayer
        EngineView* view = [[EngineView alloc] initWithFrame:frame];
        view.wantsLayer = YES;
        [ns_window setContentView:view];
        [ns_window makeFirstResponder:view];

        CocoaState* st = calloc(1, sizeof(CocoaState));
        st->ns_window = ns_window;
        st->ns_view   = view;

        window->input         = input;
        window->handle        = st;
        window->instance      = (__bridge void*)NSApp;
        window->width         = width;
        window->height        = height;
        window->should_close  = false;
        window->resized       = false;
        window->changed_mode  = false;
        window->is_minimized  = false;
        window->is_fullscreen = false;

        // --- Notificación de cierre de ventana ---
        [[NSNotificationCenter defaultCenter]
            addObserverForName:NSWindowWillCloseNotification
                        object:ns_window
                         queue:nil
                    usingBlock:^(NSNotification* note) {
                        window->should_close = true;
                    }];

        // --- Resize ---
        [[NSNotificationCenter defaultCenter]
            addObserverForName:NSWindowDidResizeNotification
                        object:ns_window
                         queue:nil
                    usingBlock:^(NSNotification* note) {
                        NSSize s = view.bounds.size;
                        window->width   = (uint32_t)s.width;
                        window->height  = (uint32_t)s.height;
                        window->resized = true;
                    }];

        [ns_window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];

        // Instalar monitores de input
        extern void window_cocoa_install_event_monitors(Window* window);
        window_cocoa_install_event_monitors(window);

        window_set_mouse_mode(window, MOUSE_MODE_CAPTURED);
    }
    return true;
}


// -----------------------------------------------------------------------------
// Poll / destroy
// -----------------------------------------------------------------------------

void window_poll_events(Window* window) {
    (void)window;
    @autoreleasepool {
        for (;;) {
            NSEvent* ev = [NSApp nextEventMatchingMask:NSEventMaskAny
                                             untilDate:[NSDate distantPast]
                                                inMode:NSDefaultRunLoopMode
                                               dequeue:YES];
            if (!ev) break;
            [NSApp sendEvent:ev];
            [NSApp updateWindows];
        }
    }
}

void window_destroy(Window* window) {
    if (!window || !window->handle) return;
    CocoaState* st = get_state(window);
    [st->ns_window close];
    if (st->key_monitor)   [NSEvent removeMonitor:st->key_monitor];
    if (st->mouse_monitor) [NSEvent removeMonitor:st->mouse_monitor];
    free(st);
    window->handle = NULL;
}


// -----------------------------------------------------------------------------
// Input: monitores globales de NSEvent
// -----------------------------------------------------------------------------

void window_cocoa_install_event_monitors(Window* window) {
    CocoaState* st = get_state(window);

    // --- Teclado ---
    st->key_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:
        (NSEventMaskKeyDown | NSEventMaskKeyUp | NSEventMaskFlagsChanged)
        handler:^NSEvent*(NSEvent* ev) {
            uint16_t keycode = ev.keyCode;
            switch (ev.type) {
                case NSEventTypeKeyDown:
                    input_on_key_down(window->input, keycode);
                    if (keycode == 0x67) {           // kVK_F11
                        window_toggle_fullscreen(window);
                    } else if (keycode == 0x30) {    // kVK_Tab
                        window_toggle_mouse_mode(window);
                    } else if (keycode == 0x35) {    // kVK_Escape
                        window_set_mouse_mode(window, MOUSE_MODE_FREE);
                    }
                    break;
                case NSEventTypeKeyUp:
                    input_on_key_up(window->input, keycode);
                    break;
                default:
                    break;
            }
            return ev;
        }];

    // --- Ratón ---
    st->mouse_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:
        (NSEventMaskMouseMoved | NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp |
         NSEventMaskRightMouseDown | NSEventMaskRightMouseUp |
         NSEventMaskOtherMouseDown | NSEventMaskOtherMouseUp |
         NSEventMaskScrollWheel)
        handler:^NSEvent*(NSEvent* ev) {
            switch (ev.type) {
                case NSEventTypeMouseMoved: {
                    if (window->mouse_mode == MOUSE_MODE_CAPTURED) {
                        input_on_mouse_delta(window->input,
                                             (int)ev.deltaX,
                                             (int)ev.deltaY);
                    } else {
                        NSPoint p = [ev locationInWindow];
                        input_on_mouse_move(window->input,
                                            (int)p.x,
                                            (int)p.y);
                    }
                    break;
                }
                case NSEventTypeLeftMouseDown:
                    input_on_mouse_button_down(window->input, 0); break;
                case NSEventTypeLeftMouseUp:
                    input_on_mouse_button_up(window->input, 0);   break;
                case NSEventTypeRightMouseDown:
                    input_on_mouse_button_down(window->input, 1); break;
                case NSEventTypeRightMouseUp:
                    input_on_mouse_button_up(window->input, 1);   break;
                case NSEventTypeOtherMouseDown:
                    input_on_mouse_button_down(window->input, 2); break;
                case NSEventTypeOtherMouseUp:
                    input_on_mouse_button_up(window->input, 2);   break;
                case NSEventTypeScrollWheel:
                    input_on_mouse_wheel(window->input,
                                         (int)ev.scrollingDeltaY);
                    break;
                default: break;
            }
            return ev;
        }];
}