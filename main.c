#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>  // Klavye tuşları için
#include <X11/XKBlib.h>  // XkbKeycodeToKeysym için
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>  // memset fonksiyonu için gerekli

// Workspace sabitleri
#define NUM_WORKSPACES 9
#define MAX_WINDOWS 100

// Tiling modu sabitleri
#define MODE_FLOATING 0    // Serbest yerleşim
#define MODE_TILING   1    // Döşeli yerleşim

// Sabitler
#define MASTER_SIZE  0.5   // Ana bölgenin ekran genişliğinin oranı

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
void rearrange_windows();
void update_screen_dimensions();
void toggle_tiling_mode();
void adjust_master_size(int delta);
void swap_master();

// Fare ile sürükleme işlemi için gerekli değişkenler
static int start_x, start_y;           // Sürükleme başlangıç koordinatları
static int orig_x, orig_y;             // Pencere orijinal koordinatları
static int orig_width, orig_height;     // Pencere orijinal boyutları
static Window dragging_window = None;   // Şu an sürüklenen pencere
static int resize_mode = 0;            // 0: taşıma, 1: boyutlandırma

// Global değişkenler
Display *display;
Window root;
static Window focused_window = None;
int window_mode = MODE_FLOATING;  // Başlangıçta serbest mod
int master_width;                 // Ana bölge genişliği (piksel)
int screen_width, screen_height;  // Ekran boyutları

// Workspace yapısı
typedef struct {
    Window windows[MAX_WINDOWS];  // Bu workspace'teki pencereler
    int window_count;            // Pencere sayısı
} Workspace;

// Global workspace değişkenleri
Workspace workspaces[NUM_WORKSPACES];
int current_workspace = 0;  // Aktif workspace (0-8)


// Tiling moduna geç
void toggle_tiling_mode() {
    window_mode = window_mode == MODE_FLOATING ? MODE_TILING : MODE_FLOATING;
    printf("Mod değiştirildi: %s\n", window_mode == MODE_FLOATING ? "Serbest" : "Döşeli");
    
    // Mevcut workspace'teki tüm pencereleri yeniden düzenle
    rearrange_windows();
}

// Ekran boyutlarını güncelle
void update_screen_dimensions() {
    Screen *screen = DefaultScreenOfDisplay(display);
    screen_width = WidthOfScreen(screen);
    screen_height = HeightOfScreen(screen);
    master_width = screen_width * MASTER_SIZE;
}

// Aktif workspace'teki pencereleri düzenle
void rearrange_windows() {
    if (window_mode == MODE_FLOATING) {
        return;  // Serbest modda pencereler yeniden düzenlenmez
    }
    
    Workspace *ws = &workspaces[current_workspace];
    int window_count = ws->window_count;
    
    if (window_count == 0) {
        return;  // Pencere yoksa işlem yapma
    }
    
    update_screen_dimensions();
    
    if (window_count == 1) {
        // Tek pencere varsa, tam ekran yap
        XMoveResizeWindow(display, ws->windows[0], 
                         0, 0, 
                         screen_width, screen_height);
        return;
    }
    
    // İlk pencere (ana) sol tarafta olacak
    XMoveResizeWindow(display, ws->windows[0], 
                     0, 0, 
                     master_width, screen_height);
    
    // Diğer pencereler sağ tarafta eşit bölünecek
    int stack_height = screen_height / (window_count - 1);
    int stack_y = 0;
    
    for (int i = 1; i < window_count; i++) {
        XMoveResizeWindow(display, ws->windows[i], 
                         master_width, stack_y, 
                         screen_width - master_width, stack_height);
        stack_y += stack_height;
    }
}

// Ana bölge genişliğini ayarla
void adjust_master_size(int delta) {
    update_screen_dimensions();
    
    // Ana bölge genişliğini değiştir (sınırlar içinde)
    int new_master_width = master_width + delta;
    if (new_master_width < screen_width * 0.1) {
        new_master_width = screen_width * 0.1;  // Minimum %10
    } else if (new_master_width > screen_width * 0.9) {
        new_master_width = screen_width * 0.9;  // Maksimum %90
    }
    
    master_width = new_master_width;
    printf("Ana bölge genişliği: %d piksel (%d%%)\n", 
           master_width, (master_width * 100) / screen_width);
    
    // Pencereleri yeni boyutlara göre düzenle
    rearrange_windows();
}

// Ana pencere ile bir sonraki pencereyi değiştir
void swap_master() {
    Workspace *ws = &workspaces[current_workspace];
    
    if (ws->window_count < 2) {
        return;  // En az 2 pencere olmalı
    }
    
    // İlk iki pencereyi değiştir
    Window temp = ws->windows[0];
    ws->windows[0] = ws->windows[1];
    ws->windows[1] = temp;
    
    // Pencereleri yeniden düzenle
    rearrange_windows();
    
    printf("Ana pencere değiştirildi\n");
}

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
    
    // Döşeli modda pencereleri yeniden düzenle
    if (window_mode == MODE_TILING) {
        rearrange_windows();
    }
    
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
    
    // Pencereler kaldırıldıktan sonra yeniden düzenle
    if (window_mode == MODE_TILING) {
        rearrange_windows();
    }
    
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

// Bir komutu çalıştır
void exec_command(const char *cmd) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Çocuk süreçte komutu çalıştır
        setsid();
        system(cmd);
        exit(EXIT_SUCCESS);
    }
    // Ebeveyn süreç devam eder
}

// Aktif pencereyi kapat
void close_window(Window window) {
    if (window == None || window == root) {
        return;
    }
    
    // Pencereye kapatma mesajı gönder
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = ClientMessage;
    ev.xclient.window = window;
    ev.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    ev.xclient.data.l[1] = CurrentTime;
    
    XSendEvent(display, window, False, NoEventMask, &ev);
    printf("Pencere kapatma isteği gönderildi: %ld\n", window);
}

// Fonksiyon prototipi
void move_window_to_workspace(Window window, int from_ws, int to_ws);

// Pencereyi başka bir workspace'e taşı
void move_window_to_workspace(Window window, int from_ws, int to_ws) {
    if (from_ws < 0 || from_ws >= NUM_WORKSPACES ||
        to_ws < 0 || to_ws >= NUM_WORKSPACES ||
        from_ws == to_ws || window == None) {
        return;
    }
    
    // Pencereyi eski workspace'den kaldır
    remove_window_from_workspace(window, from_ws);
    
    // Pencereyi yeni workspace'e ekle
    add_window_to_workspace(window, to_ws);
    
    // Aktif workspace değiştiyse, pencereyi sakla/göster
    if (current_workspace != to_ws) {
        XUnmapWindow(display, window);
    } else {
        XMapWindow(display, window);
    }
    
    // Döşeme modunda eski ve yeni workspace'teki pencereleri düzenle
    if (window_mode == MODE_TILING) {
        // Geçici olarak workspace'i değiştir ve düzenle
        int temp_ws = current_workspace;
        
        if (from_ws != current_workspace) {
            current_workspace = from_ws;
            rearrange_windows();
        }
        
        if (to_ws != from_ws) {
            current_workspace = to_ws;
            rearrange_windows();
        }
        
        current_workspace = temp_ws;
    }
    
    printf("Pencere %ld workspace %d'den %d'e taşındı\n", 
           window, from_ws + 1, to_ws + 1);
}

// Klavye olaylarını işle
void handle_key_press(XKeyEvent *event) {
    KeySym keysym = XkbKeycodeToKeysym(display, event->keycode, 0, 0);
    
    // Alt+Shift kombinasyonlarını kontrol et
    if ((event->state & Mod1Mask) && (event->state & ShiftMask)) {
        if (keysym >= XK_1 && keysym <= XK_9) {
            // Alt + Shift + 1-9: Aktif pencereyi başka workspace'e taşı
            if (focused_window != None) {
                int target_workspace = keysym - XK_1;
                printf("Alt + Shift + %d tuşuna basıldı\n", target_workspace + 1);
                move_window_to_workspace(focused_window, current_workspace, target_workspace);
            }
        }
    }
    // Sadece Alt tuşu kombinasyonlarını kontrol et
    else if (event->state & Mod1Mask) {
        if (keysym >= XK_1 && keysym <= XK_9) {
            // Alt + 1-9: Workspace değiştir
            int workspace = keysym - XK_1;
            printf("Alt + %d tuşuna basıldı\n", workspace + 1);
            switch_workspace(workspace);
        }
        else if (keysym == XK_d) {
            // Alt + d: dmenu çalıştır
            printf("dmenu çalıştırılıyor...\n");
            exec_command("dmenu_run -l 10 -p 'Uygulama seç:' -fn 'Terminus-13' -nb '#242933' -sb '#1b1f26'");
        }
        else if (keysym == XK_q) {
            // Alt + q: Aktif pencereyi kapat
            if (focused_window != None) {
                printf("Aktif pencere kapatılıyor: %ld\n", focused_window);
                close_window(focused_window);
            }
        }
        else if (keysym == XK_t) {
            // Alt + t: Tiling modunu değiştir
            toggle_tiling_mode();
        }
        else if (keysym == XK_h) {
            // Alt + h: Ana bölge genişliğini azalt
            adjust_master_size(-50);
        }
        else if (keysym == XK_l) {
            // Alt + l: Ana bölge genişliğini arttır
            adjust_master_size(50);
        }
        else if (keysym == XK_Return) {
            // Alt + Enter: Ana pencere ile değiştir
            swap_master();
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

    // Alt + tuş kombinasyonlarını yakala
    XGrabKey(display, 
             AnyKey,
             Mod1Mask,
             root,
             True,
             GrabModeAsync,
             GrabModeAsync);

    // Özel tuşları ayrıca ayarla
    KeyCode d_key = XKeysymToKeycode(display, XK_d);
    XGrabKey(display, d_key, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    
    KeyCode q_key = XKeysymToKeycode(display, XK_q);
    XGrabKey(display, q_key, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);

    // Alt+Shift+1-9 tuşlarını yakala
    for (int i = XK_1; i <= XK_9; i++) {
        KeyCode keycode = XKeysymToKeycode(display, i);
        XGrabKey(display, 
                 keycode,
                 Mod1Mask | ShiftMask,  // Alt+Shift
                 root,
                 True,
                 GrabModeAsync,
                 GrabModeAsync);
    }

    // Ekran boyutlarını başlat
    update_screen_dimensions();

    // Özel tuşları ayarla
    KeyCode t_key = XKeysymToKeycode(display, XK_t);
    XGrabKey(display, t_key, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    
    KeyCode h_key = XKeysymToKeycode(display, XK_h);
    XGrabKey(display, h_key, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    
    KeyCode l_key = XKeysymToKeycode(display, XK_l);
    XGrabKey(display, l_key, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    
    KeyCode return_key = XKeysymToKeycode(display, XK_Return);
    XGrabKey(display, return_key, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);

    XSetErrorHandler(error_handler);

    printf("Pencere yöneticisi başlatıldı...\n");
    printf("Alt + 1-9: Workspace değiştir\n");
    printf("Alt + Shift + 1-9: Aktif pencereyi belirtilen workspace'e taşı\n");
    printf("Alt + d: dmenu çalıştır\n");
    printf("Alt + q: Aktif pencereyi kapat\n");
    printf("Alt + t: Tiling/Floating mod değiştir\n");
    printf("Alt + h/l: Ana bölge genişliğini azalt/arttır\n");
    printf("Alt + Enter: Ana pencere ile değiştir\n");

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


