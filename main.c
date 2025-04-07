#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>  // Klavye tuşları için
#include <X11/XKBlib.h>  // XkbKeycodeToKeysym için
#include <stdio.h>
#include <stdlib.h>

// Workspace sabitleri
#define NUM_WORKSPACES 9
#define MAX_WINDOWS 100

// Global değişkenler
Display *display;
Window root;
static Window focused_window = None;

// Workspace yapısı
typedef struct {
    Window windows[MAX_WINDOWS];  // Bu workspace'teki pencereler
    int window_count;            // Pencere sayısı
} Workspace;

// Global workspace değişkenleri
Workspace workspaces[NUM_WORKSPACES];
int current_workspace = 0;  // Aktif workspace (0-8)

// Workspace yönetimi fonksiyonları
void init_workspaces() {
    for (int i = 0; i < NUM_WORKSPACES; i++) {
        workspaces[i].window_count = 0;
        for (int j = 0; j < MAX_WINDOWS; j++) {
            workspaces[i].windows[j] = None;
        }
    }
    printf("Workspaces başlatıldı\n");
}

// Pencereyi workspace'e ekle
void add_window_to_workspace(Window w, int workspace) {
    if (workspace < 0 || workspace >= NUM_WORKSPACES) return;
    if (workspaces[workspace].window_count >= MAX_WINDOWS) return;
    
    // Pencere zaten bu workspace'te mi kontrol et
    for (int i = 0; i < workspaces[workspace].window_count; i++) {
        if (workspaces[workspace].windows[i] == w) return;
    }
    
    workspaces[workspace].windows[workspaces[workspace].window_count++] = w;
    printf("Pencere %ld workspace %d'e eklendi\n", w, workspace + 1);
}

// Pencereyi workspace'den kaldır
void remove_window_from_workspace(Window w, int workspace) {
    if (workspace < 0 || workspace >= NUM_WORKSPACES) return;
    
    for (int i = 0; i < workspaces[workspace].window_count; i++) {
        if (workspaces[workspace].windows[i] == w) {
            // Pencereyi listeden çıkar ve diğerlerini kaydır
            for (int j = i; j < workspaces[workspace].window_count - 1; j++) {
                workspaces[workspace].windows[j] = workspaces[workspace].windows[j + 1];
            }
            workspaces[workspace].window_count--;
            printf("Pencere %ld workspace %d'den kaldırıldı\n", w, workspace + 1);
            break;
        }
    }
}

// Workspace'i değiştir
void switch_workspace(int new_workspace) {
    if (new_workspace < 0 || new_workspace >= NUM_WORKSPACES) return;
    if (new_workspace == current_workspace) return;

    printf("Workspace değiştiriliyor: %d -> %d\n", current_workspace + 1, new_workspace + 1);

    // Mevcut workspace'deki pencereleri gizle
    for (int i = 0; i < workspaces[current_workspace].window_count; i++) {
        Window w = workspaces[current_workspace].windows[i];
        if (w != None) {
            XUnmapWindow(display, w);
            printf("Pencere gizlendi: %ld\n", w);
        }
    }

    // Yeni workspace'e geç
    current_workspace = new_workspace;

    // Yeni workspace'deki pencereleri göster
    for (int i = 0; i < workspaces[current_workspace].window_count; i++) {
        Window w = workspaces[current_workspace].windows[i];
        if (w != None) {
            XMapWindow(display, w);
            printf("Pencere gösterildi: %ld\n", w);
        }
    }

    // Değişiklikleri hemen uygula
    XSync(display, False);
}

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
                StructureNotifyMask |
                KeyPressMask);  // Klavye olaylarını da dinle
    
    // Pencere konumunu ve boyutunu ayarla
    XMoveResizeWindow(display, event->window,
                     attrs.x,
                     attrs.y,
                     attrs.width,
                     attrs.height);
    
    // Pencereyi mevcut workspace'e ekle
    add_window_to_workspace(event->window, current_workspace);
    
    // Yeni pencereyi otomatik odakla
    focus_window(event->window);
    
    printf("Yeni pencere workspace %d'e eklendi: %ld\n", current_workspace + 1, event->window);
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
    // Pencereyi mevcut workspace'den kaldır
    remove_window_from_workspace(event->window, current_workspace);
    
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
    printf("Pencere workspace %d'den kaldırıldı: %ld\n", current_workspace + 1, event->window);
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

// Klavye olaylarını işle
void handle_key_press(XKeyEvent *event) {
    KeySym keysym = XkbKeycodeToKeysym(display, event->keycode, 0, 0);
    
    if (event->state & Mod1Mask) {  // Alt tuşu
        if (keysym >= XK_1 && keysym <= XK_9) {
            int workspace = keysym - XK_1;  // 0-8 arası indeks
            printf("Alt + %d tuşuna basıldı\n", workspace + 1);
            switch_workspace(workspace);
        }
    }
}

int main() {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "X sunucusuna bağlanılamadı.\n");
        return 1;
    }

    root = DefaultRootWindow(display);

    // Workspace'leri başlat
    init_workspaces();

    // Root pencere için olay maskesini güncelle
    XSelectInput(display, root,
                SubstructureRedirectMask |
                SubstructureNotifyMask |
                ButtonPressMask |
                ButtonReleaseMask |
                PointerMotionMask |
                KeyPressMask);

    // Klavye olaylarını root pencereye yönlendir
    XGrabKey(display, 
             AnyKey,           // Tüm tuşları yakala
             Mod1Mask,         // Alt tuşu ile birlikte
             root,             // Root pencerede
             True,             // Owner events
             GrabModeAsync,    // Pointer grab modu
             GrabModeAsync);   // Keyboard grab modu

    // Alt + 1-9 tuşlarını özel olarak yakala
    for (int i = XK_1; i <= XK_9; i++) {
        KeyCode keycode = XKeysymToKeycode(display, i);
        XGrabKey(display, 
                 keycode,
                 Mod1Mask,
                 root,
                 True,
                 GrabModeAsync,
                 GrabModeAsync);
    }

    XSetErrorHandler(error_handler);

    printf("Pencere yöneticisi başlatıldı...\n");
    printf("Alt + 1-9: Workspace değiştir\n");

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
            case KeyPress:
                handle_key_press(&event.xkey);
                break;
            case EnterNotify:
                if (event.xcrossing.mode == NotifyNormal) {
                    focus_window(event.xcrossing.window);
                }
                break;
        }
    }

    // Program sonunda temizlik
    XUngrabKey(display, AnyKey, AnyModifier, root);
    XCloseDisplay(display);
    return 0;
}


