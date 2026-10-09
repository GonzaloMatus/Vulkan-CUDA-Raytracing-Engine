// window_cocoa.m
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <stdlib.h>
#include "window.h"

// -----------------------------------------------------------------------------
// Traducción de keyCode (NSEvent, macOS) → Virtual-Key code (Windows)
// -----------------------------------------------------------------------------
// input.c indexa el array de teclas con VK codes (0..255). En Windows eso es
// lo que entrega WM_KEYDOWN. En macOS, NSEvent.keyCode es un código distinto
// (HID usage). Esta tabla hace de puente. Devuelve -1 si la tecla no está
// mapeada (input.c ignora códigos fuera de rango).
static int nsevent_keycode_to_vk(uint16_t kc) {
    switch (kc) {
        // Letras (A..Z)
        case 0x00: return 0x41;  // A
        case 0x0B: return 0x42;  // B
        case 0x08: return 0x43;  // C
        case 0x02: return 0x44;  // D
        case 0x0E: return 0x45;  // E
        case 0x03: return 0x46;  // F
        case 0x05: return 0x47;  // G
        case 0x04: return 0x48;  // H
        case 0x22: return 0x49;  // I
        case 0x26: return 0x4A;  // J
        case 0x28: return 0x4B;  // K
        case 0x25: return 0x4C;  // L
        case 0x2E: return 0x4D;  // M
        case 0x2D: return 0x4E;  // N
        case 0x1F: return 0x4F;  // O
        case 0x23: return 0x50;  // P
        case 0x0C: return 0x51;  // Q
        case 0x0F: return 0x52;  // R
        case 0x01: return 0x53;  // S
        case 0x11: return 0x54;  // T
        case 0x20: return 0x55;  // U
        case 0x09: return 0x56;  // V
        case 0x0D: return 0x57;  // W
        case 0x07: return 0x58;  // X
        case 0x10: return 0x59;  // Y
        case 0x06: return 0x5A;  // Z

        // Números (fila superior)
        case 0x1D: return 0x30;  // 0
        case 0x12: return 0x31;  // 1
        case 0x13: return 0x32;  // 2
        case 0x14: return 0x33;  // 3
        case 0x15: return 0x34;  // 4
        case 0x17: return 0x35;  // 5
        case 0x16: return 0x36;  // 6
        case 0x1A: return 0x37;  // 7
        case 0x1C: return 0x38;  // 8
        case 0x19: return 0x39;  // 9

        // Especiales
        case 0x35: return 0x1B;  // ESC
        case 0x30: return 0x09;  // TAB
        case 0x24: return 0x0D;  // RETURN
        case 0x33: return 0x08;  // BACKSPACE
        case 0x31: return 0x20;  // SPACE
        case 0x75: return 0x2E;  // DELETE (forward delete)

        // Modificadores
        case 0x38: return 0x10;  // LSHIFT
        case 0x3C: return 0x10;  // RSHIFT → mismo VK que LSHIFT
        case 0x3B: return 0x11;  // LCTRL
        case 0x3E: return 0x11;  // RCTRL
        case 0x3A: return 0x12;  // LALT
        case 0x3D: return 0x12;  // RALT
        case 0x37: return 0x5B;  // LCMD → VK_LWIN
        case 0x36: return 0x5C;  // RCMD → VK_RWIN
        case 0x39: return 0x14;  // CAPSLOCK

        // Flechas
        case 0x7B: return 0x25;  // LEFT
        case 0x7C: return 0x27;  // RIGHT
        case 0x7D: return 0x28;  // DOWN
        case 0x7E: return 0x26;  // UP

        // F-keys
        case 0x7A: return 0x70;  // F1
        case 0x78: return 0x71;  // F2
        case 0x63: return 0x72;  // F3
        case 0x76: return 0x73;  // F4
        case 0x60: return 0x74;  // F5
        case 0x61: return 0x75;  // F6
        case 0x62: return 0x76;  // F7
        case 0x64: return 0x77;  // F8
        case 0x65: return 0x78;  // F9
        case 0x6D: return 0x79;  // F10
        case 0x67: return 0x7A;  // F11
        case 0x6F: return 0x7B;  // F12

        default:   return -1;
    }
}


// -----------------------------------------------------------------------------
// Vista respaldada por CAMetalLayer
// -----------------------------------------------------------------------------

@interface EngineView : NSView
@end

@implementation EngineView
- (CALayer*)makeBackingLayer { return [CAMetalLayer layer]; }
- (BOOL)wantsUpdateLayer { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)isOpaque { return YES; }
@end


// -----------------------------------------------------------------------------
// Delegate para interceptar el cierre
// -----------------------------------------------------------------------------

@interface EngineWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) Window* c_window;
@end

@implementation EngineWindowDelegate
- (BOOL)windowShouldClose:(NSWindow*)sender {
    (void)sender;
    if (self.c_window) self.c_window->should_close = true;
    return NO;
}
@end


// -----------------------------------------------------------------------------
// Estado nativo
// -----------------------------------------------------------------------------

typedef struct {
    NSWindow*              ns_window;
    EngineView*            ns_view;
    EngineWindowDelegate*  delegate;
    id                     key_monitor;
    id                     mouse_monitor;
    id                     resize_observer;
} CocoaState;

static CocoaState* get_state(Window* w) { return (CocoaState*)w->handle; }

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
        [ns_window setReleasedWhenClosed:NO];

        EngineView* view = [[EngineView alloc] initWithFrame:frame];
        view.wantsLayer = YES;
        [ns_window setContentView:view];
        [ns_window makeFirstResponder:view];

        CocoaState* st = calloc(1, sizeof(CocoaState));
        st->ns_window = ns_window;
        st->ns_view   = view;

        st->delegate = [[EngineWindowDelegate alloc] init];
        st->delegate.c_window = window;
        [ns_window setDelegate:st->delegate];

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

        st->resize_observer = [[NSNotificationCenter defaultCenter]
            addObserverForName:NSWindowDidResizeNotification
                        object:ns_window
                         queue:nil
                    usingBlock:^(NSNotification* note) {
                        (void)note;
                        NSSize s = view.bounds.size;
                        CGFloat scale = ns_window.backingScaleFactor;
                        CAMetalLayer* layer = (CAMetalLayer*)view.layer;
                        layer.drawableSize = CGSizeMake(s.width * scale, s.height * scale);
                        window->width   = (uint32_t)s.width;
                        window->height  = (uint32_t)s.height;
                        window->resized = true;
                    }];

        [ns_window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];

        extern void window_cocoa_install_event_monitors(Window* window);
        window_cocoa_install_event_monitors(window);

        window_set_mouse_mode(window, MOUSE_MODE_CAPTURED);
    }
    return true;
}

void window_poll_events(Window* window) {
    window->resized      = false;
    window->changed_mode = false;

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

    if (st->resize_observer) {
        [[NSNotificationCenter defaultCenter] removeObserver:st->resize_observer];
        st->resize_observer = nil;
    }
    if (st->key_monitor)   [NSEvent removeMonitor:st->key_monitor];
    if (st->mouse_monitor) [NSEvent removeMonitor:st->mouse_monitor];

    [st->ns_window setDelegate:nil];
    st->delegate = nil;

    [st->ns_window close];

    free(st);
    window->handle = NULL;
}


// -----------------------------------------------------------------------------
// Input: monitores de NSEvent
// -----------------------------------------------------------------------------

void window_cocoa_install_event_monitors(Window* window) {
    CocoaState* st = get_state(window);

    st->key_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:
        (NSEventMaskKeyDown | NSEventMaskKeyUp | NSEventMaskFlagsChanged)
        handler:^NSEvent*(NSEvent* ev) {

            // Dejar pasar los atajos con Cmd (Cmd+Q, Cmd+W, Cmd+H, etc.)
            // para que macOS los procese normalmente.
            if (ev.modifierFlags & NSEventModifierFlagCommand) return ev;

            uint16_t kc = ev.keyCode;

            switch (ev.type) {
                case NSEventTypeKeyDown: {
                    int vk = nsevent_keycode_to_vk(kc);
                    if (vk < 0) return ev;   // tecla no mapeada: dejar pasar
                    input_on_key_down(window->input, vk);

                    if (vk == 0x7A) {          // F11 → fullscreen
                        window_toggle_fullscreen(window);
                    } else if (vk == 0x09) {   // TAB → toggle mouse mode
                        window_toggle_mouse_mode(window);
                    } else if (vk == 0x1B) {   // ESC → liberar mouse
                        window_set_mouse_mode(window, MOUSE_MODE_FREE);
                    }

                    return nil;   // consumir el evento → sin beep
                }

                case NSEventTypeKeyUp: {
                    int vk = nsevent_keycode_to_vk(kc);
                    if (vk < 0) return ev;
                    input_on_key_up(window->input, vk);
                    return nil;
                }

                case NSEventTypeFlagsChanged: {
                    // Los modificadores (Shift, Ctrl, Alt, Cmd, CapsLock)
                    // solo generan FlagsChanged, no KeyDown/KeyUp.
                    NSEventModifierFlags flags =
                        ev.modifierFlags & NSEventModifierFlagDeviceIndependentFlagsMask;

                    int  vk      = -1;
                    bool is_down = false;

                    switch (kc) {
                        case 0x38:   // Left Shift
                        case 0x3C:   // Right Shift
                            vk = 0x10;
                            is_down = (flags & NSEventModifierFlagShift) != 0;
                            break;

                        case 0x3B:   // Left Control
                        case 0x3E:   // Right Control
                            vk = 0x11;
                            is_down = (flags & NSEventModifierFlagControl) != 0;
                            break;

                        case 0x3A:   // Left Option
                        case 0x3D:   // Right Option
                            vk = 0x12;
                            is_down = (flags & NSEventModifierFlagOption) != 0;
                            break;

                        case 0x37:   // Left Command
                            vk = 0x5B;
                            is_down = (flags & NSEventModifierFlagCommand) != 0;
                            break;

                        case 0x36:   // Right Command
                            vk = 0x5C;
                            is_down = (flags & NSEventModifierFlagCommand) != 0;
                            break;

                        case 0x39:   // Caps Lock
                            vk = 0x14;
                            is_down = (flags & NSEventModifierFlagCapsLock) != 0;
                            break;

                        default:
                            return ev;   // modificador desconocido, dejar pasar
                    }

                    if (vk < 0) return ev;

                    if (is_down) input_on_key_down(window->input, vk);
                    else         input_on_key_up(window->input, vk);

                    return nil;
                }

                default:
                    return ev;
            }
        }];

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
                case NSEventTypeLeftMouseDown:  input_on_mouse_button_down(window->input, 0); break;
                case NSEventTypeLeftMouseUp:    input_on_mouse_button_up(window->input, 0);   break;
                case NSEventTypeRightMouseDown: input_on_mouse_button_down(window->input, 1); break;
                case NSEventTypeRightMouseUp:   input_on_mouse_button_up(window->input, 1);   break;
                case NSEventTypeOtherMouseDown: input_on_mouse_button_down(window->input, 2); break;
                case NSEventTypeOtherMouseUp:   input_on_mouse_button_up(window->input, 2);   break;
                case NSEventTypeScrollWheel:    input_on_mouse_wheel(window->input,
                                                                     (int)ev.scrollingDeltaY); break;
                default: break;
            }
            return ev;
        }];
}