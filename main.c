#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>

Display *display;
Window root;

// Fonksiyon prototipleri
void focus_window(Window window);
void handle_map_request(XMapRequestEvent *event);
void handle_button_press(XButtonEvent *event);
void handle_destroy_notify(XDestroyWindowEvent *event);
void handle_configure_request(XConfigureRequestEvent *event);
void start_move(XButtonEvent *event);
void start_resize(XButtonEvent *event);
void handle_motion(XMotionEvent *event);
void stop_drag(XButtonEvent *event);

// Fare ile sürükleme işlemi için gerekli değişkenler
static int start_x, start_y;           // Sürükleme başlangıç koordinatları
static int orig_x, orig_y;             // Pencere orijinal koordinatları
static int orig_width, orig_height;     // Pencere orijinal boyutları
static Window dragging_window = None;   // Şu an sürüklenen pencere
static int resize_mode = 0;            // 0: taşıma, 1: boyutlandırma

// Global değişkenlere ekle
static Window focused_window = None;

// Pencereyi odakla
void focus_window(Window window) {
    if (window == None || window == root) {
        return;
    }

    // Önceki odaklanmış pencereyi temizle
    if (focused_window != None) {
        XSetWindowBorder(display, focused_window, 0x000000);
    }

    // Yeni pencereyi odakla
    focused_window = window;
    XSetWindowBorder(display, window, 0x4C7899);  // Mavi tonunda bir kenarlık
    XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(display, window);  // Pencereyi öne getir
}

// Yeni pencere oluşturma isteğini işle
void handle_map_request(XMapRequestEvent *event) {
    // Pencereyi görünür yap
    XMapWindow(display, event->window);
    
    // Pencere özelliklerini al
    XWindowAttributes attrs;
    XGetWindowAttributes(display, event->window, &attrs);
    
    // Pencereyi yönetmeye başla
    XSelectInput(display, event->window,
                EnterWindowMask |
                FocusChangeMask |
                PropertyChangeMask |
                StructureNotifyMask);
    
    // Pencere konumunu ve boyutunu ayarla
    XMoveResizeWindow(display, event->window,
                     attrs.x,
                     attrs.y,
                     attrs.width,
                     attrs.height);
    
    // Yeni pencereyi otomatik odakla
    focus_window(event->window);
    
    printf("Yeni pencere oluşturuldu ve odaklandı: %ld\n", event->window);
}

// Pencere tıklama olayını işle
void handle_button_press(XButtonEvent *event) {
    if (event->button == Button1) {  // Sol tık
        // Pencereyi odakla ve taşımaya başla
        focus_window(event->window);
        start_move(event);
    } else if (event->button == Button3) {  // Sağ tık
        // Pencereyi odakla ve boyutlandırmaya başla
        focus_window(event->window);
        start_resize(event);
    }
}

// Pencere yok edildiğinde odağı temizle
void handle_destroy_notify(XDestroyWindowEvent *event) {
    if (event->window == focused_window) {
        focused_window = None;
        // Fare konumundaki pencereye odaklan
        Window root_return, child_return;
        int root_x, root_y, win_x, win_y;
        unsigned int mask_return;
        
        XQueryPointer(display, root,
                     &root_return, &child_return,
                     &root_x, &root_y,
                     &win_x, &win_y,
                     &mask_return);
                     
        if (child_return != None) {
            focus_window(child_return);
        }
    }
    printf("Pencere yok edildi: %ld\n", event->window);
}

// Pencere yapılandırma değişikliklerini işle
void handle_configure_request(XConfigureRequestEvent *event) {
    XWindowChanges changes;
    
    // İstenen değişiklikleri uygula
    changes.x = event->x;
    changes.y = event->y;
    changes.width = event->width;
    changes.height = event->height;
    changes.border_width = event->border_width;
    changes.sibling = event->above;
    changes.stack_mode = event->detail;
    
    XConfigureWindow(display, event->window, event->value_mask, &changes);
    printf("Pencere yapılandırması güncellendi: %ld\n", event->window);
}

// Hata işleyici
int error_handler(Display *display, XErrorEvent *e) {
    char error_text[256];
    XGetErrorText(display, e->error_code, error_text, sizeof(error_text));
    fprintf(stderr, "X Hatası: %s\n", error_text);
    return 0;
}

// Pencereyi taşımaya başla
void start_move(XButtonEvent *event) {
    Window returned_root, returned_parent;
    Window *children;
    unsigned int nchildren;
    XWindowAttributes attrs;

    // Tıklanan noktadaki pencereyi bul
    XQueryTree(display, root, &returned_root, &returned_parent, &children, &nchildren);
    if (children) {
        dragging_window = children[nchildren - 1];  // En üstteki pencere
        XFree(children);
    }

    if (dragging_window != None) {
        XGetWindowAttributes(display, dragging_window, &attrs);
        start_x = event->x_root;
        start_y = event->y_root;
        orig_x = attrs.x;
        orig_y = attrs.y;
        orig_width = attrs.width;
        orig_height = attrs.height;

        // Fare ikonunu değiştir
        Cursor cursor = XCreateFontCursor(display, XC_fleur);
        XGrabPointer(display, root, True,
                    ButtonMotionMask | ButtonReleaseMask,
                    GrabModeAsync, GrabModeAsync,
                    root, cursor, CurrentTime);
        XFreeCursor(display, cursor);
    }
}

// Pencereyi boyutlandırmaya başla
void start_resize(XButtonEvent *event) {
    start_move(event);  // Aynı başlangıç işlemleri
    if (dragging_window != None) {
        resize_mode = 1;
        // Boyutlandırma için fare ikonunu değiştir
        Cursor cursor = XCreateFontCursor(display, XC_sizing);
        XGrabPointer(display, root, True,
                    ButtonMotionMask | ButtonReleaseMask,
                    GrabModeAsync, GrabModeAsync,
                    root, cursor, CurrentTime);
        XFreeCursor(display, cursor);
    }
}

// Fare hareketi işleme
void handle_motion(XMotionEvent *event) {
    if (dragging_window != None) {
        int xdiff = event->x_root - start_x;
        int ydiff = event->y_root - start_y;

        if (resize_mode) {
            // Boyutlandırma
            int new_width = orig_width + xdiff;
            int new_height = orig_height + ydiff;
            
            // Minimum boyut kontrolü
            if (new_width < 100) new_width = 100;
            if (new_height < 100) new_height = 100;

            XResizeWindow(display, dragging_window, new_width, new_height);
        } else {
            // Taşıma
            XMoveWindow(display, dragging_window,
                       orig_x + xdiff,
                       orig_y + ydiff);
        }
    }
}

// Sürükleme işlemini bitir
void stop_drag(XButtonEvent *event) {
    if (dragging_window != None) {
        XUngrabPointer(display, CurrentTime);
        dragging_window = None;
        resize_mode = 0;
    }
}

int main() {
    // X sunucusuna bağlan
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "X sunucusuna bağlanılamadı.\n");
        return 1;
    }

    // Root pencereyi al
    root = DefaultRootWindow(display);

    // Root pencere için olay maskesini güncelle
    XSelectInput(display, root,
                SubstructureRedirectMask |
                SubstructureNotifyMask |
                ButtonPressMask |
                ButtonReleaseMask |
                PointerMotionMask);

    // Error handler'ı ayarla (X sunucusu başka bir pencere yöneticisi çalışıyorsa hata alırız)
    XSetErrorHandler(error_handler);

    printf("Pencere yöneticisi başlatıldı...\n");

    // Ana döngü
    XEvent event;
    while (1) {
        XNextEvent(display, &event);
        
        switch (event.type) {
            case MapRequest:
                handle_map_request(&event.xmaprequest);
                break;
            case DestroyNotify:
                handle_destroy_notify(&event.xdestroywindow);
                break;
            case ConfigureRequest:
                handle_configure_request(&event.xconfigurerequest);
                break;
            case ButtonPress:
                handle_button_press(&event.xbutton);
                break;
            case ButtonRelease:
                stop_drag(&event.xbutton);
                break;
            case MotionNotify:
                handle_motion(&event.xmotion);
                break;
            case EnterNotify:  // Fare pencereye girdiğinde
                if (event.xcrossing.mode == NotifyNormal) {
                    focus_window(event.xcrossing.window);
                }
                break;
        }
    }

    // Temizlik
    XCloseDisplay(display);
    return 0;
}


