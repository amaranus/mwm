#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>  // Klavye tuşları için
#include <X11/XKBlib.h>  // XkbKeycodeToKeysym için
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>  // memset fonksiyonu için gerekli
#include <sys/time.h>  // gettimeofday için
#include <X11/Xatom.h>
#include <X11/Xutil.h>

// Workspace sabitleri
#define NUM_WORKSPACES 5
#define MAX_WINDOWS 100
#define BORDER_WIDTH 2  // Pencere kenarlık kalınlığı

// Tiling modu sabitleri
#define MODE_FLOATING 0    // Serbest yerleşim
#define MODE_TILING   1    // Döşeli yerleşim

// Sabitler
#define MASTER_SIZE  0.5   // Ana bölgenin ekran genişliğinin oranı
#define OUTER_GAP 10    // Ekran kenarlarıyla pencereler arası boşluk
#define INNER_GAP 10    // Pencereler arası boşluk
#define TERMINAL "alacritty" // Terminal programı
#define LAUNCHER "dmenu_run -l 10 -p 'Uygulama seç:' -fn 'Terminus-13' -nb '#353535' -sb '#0a0a0a'" // Uygulama seçici
#define WS_NOTIFICATION_BG 0x353535 // Workspace notification background color
#define WS_NOTIFICATION_FG 0xD8DEE9 // Workspace notification foreground color
#define WS_NOTIFICATION_BORDER 0x88C0D0 // Workspace notification border color
#define ACTIVE_WINDOW_BORDER_FG 0x4C7899
#define WINDOW_BORDER_FG 0x000000
#define MODKEY Mod1Mask

// Bar sabitleri
#define BAR_HEIGHT 30  // Bar yüksekliği
#define BAR_POSITION_TOP 1  // 1: üstte, 0: altta

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
void adjust_master_size(float delta_percent);
void swap_master();
void toggle_gaps();  // Bu satırı ekleyin
void adjust_gaps(int outer_delta, int inner_delta);
void focus_next_window();
void create_notification_window();
void show_workspace_notification(int workspace_num);
void check_notification_timeout();
void init_atoms();
void update_workspace_properties();
void handle_strut_properties(Window window);
void move_window_to_workspace(Window window, int from_ws, int to_ws);

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
int outer_gap = OUTER_GAP;
int inner_gap = INNER_GAP;
int gaps_enabled = 1;   // Boşluklar varsayılan olarak açık
float master_size_percent = 50.0;  // Ana bölge genişliği yüzdesi (başlangıçta %50)
int is_switching_workspace = 0;  // Workspace değişimi sırasında bayrak

// Workspace yapısı
typedef struct {
    Window windows[MAX_WINDOWS];  // Bu workspace'teki pencereler
    int window_count;            // Pencere sayısı
    int mode;                    // Bu workspace'in modu (MODE_FLOATING veya MODE_TILING)
} Workspace;

// Global workspace değişkenleri
Workspace workspaces[NUM_WORKSPACES];
int current_workspace = 0;  // Aktif workspace (0-8)

// Tuş kodları için yapı tanımı
typedef struct {
    KeyCode a_key;
    KeyCode b_key;
    KeyCode c_key;
    KeyCode d_key;
    KeyCode e_key;
    KeyCode f_key;
    KeyCode g_key;          
    KeyCode h_key;
    KeyCode i_key;
    KeyCode j_key;         
    KeyCode k_key;        
    KeyCode l_key;
    KeyCode m_key;
    KeyCode n_key;
    KeyCode o_key;
    KeyCode p_key;
    KeyCode q_key;
    KeyCode r_key;
    KeyCode s_key;
    KeyCode t_key;
    KeyCode u_key;
    KeyCode v_key;
    KeyCode w_key;
    KeyCode x_key;
    KeyCode y_key;
    KeyCode z_key;
    KeyCode return_key;
    KeyCode volume_raise_key;
    KeyCode volume_lower_key;
    KeyCode volume_mute_key;
    KeyCode left_key;    
    KeyCode right_key;   
    KeyCode up_key;  
    KeyCode down_key;
    KeyCode tab_key;
} KeyBindings;

// Global tuş kodları değişkeni
KeyBindings keys;

// Tuş kodlarını başlat fonksiyonu (main içinde çağrılacak)
void init_keybindings() {
    keys.a_key = XKeysymToKeycode(display, XK_a);
    keys.b_key = XKeysymToKeycode(display, XK_b);
    keys.c_key = XKeysymToKeycode(display, XK_c);
    keys.d_key = XKeysymToKeycode(display, XK_d);
    keys.e_key = XKeysymToKeycode(display, XK_e);
    keys.f_key = XKeysymToKeycode(display, XK_f);
    keys.g_key = XKeysymToKeycode(display, XK_g);
    keys.h_key = XKeysymToKeycode(display, XK_h);
    keys.i_key = XKeysymToKeycode(display, XK_i);
    keys.j_key = XKeysymToKeycode(display, XK_j);
    keys.k_key = XKeysymToKeycode(display, XK_k);
    keys.l_key = XKeysymToKeycode(display, XK_l);
    keys.m_key = XKeysymToKeycode(display, XK_m);
    keys.n_key = XKeysymToKeycode(display, XK_n);
    keys.o_key = XKeysymToKeycode(display, XK_o);
    keys.p_key = XKeysymToKeycode(display, XK_p);
    keys.q_key = XKeysymToKeycode(display, XK_q);
    keys.r_key = XKeysymToKeycode(display, XK_r);
    keys.s_key = XKeysymToKeycode(display, XK_s);
    keys.t_key = XKeysymToKeycode(display, XK_t);
    keys.u_key = XKeysymToKeycode(display, XK_u);
    keys.v_key = XKeysymToKeycode(display, XK_v);
    keys.w_key = XKeysymToKeycode(display, XK_w);
    keys.x_key = XKeysymToKeycode(display, XK_x);
    keys.y_key = XKeysymToKeycode(display, XK_y);
    keys.z_key = XKeysymToKeycode(display, XK_z);    
    keys.return_key = XKeysymToKeycode(display, XK_Return);
    keys.volume_raise_key = XKeysymToKeycode(display, XStringToKeysym("XF86AudioRaiseVolume"));
    keys.volume_lower_key = XKeysymToKeycode(display, XStringToKeysym("XF86AudioLowerVolume"));
    keys.volume_mute_key = XKeysymToKeycode(display, XStringToKeysym("XF86AudioMute"));
    keys.left_key = XKeysymToKeycode(display, XK_Left);
    keys.right_key = XKeysymToKeycode(display, XK_Right);
    keys.up_key = XKeysymToKeycode(display, XK_Up);
    keys.down_key = XKeysymToKeycode(display, XK_Down);
    keys.tab_key = XKeysymToKeycode(display, XK_Tab);
}

// Tuş yakalama fonksiyonu
void grab_keys() {
    // Alt + tuş kombinasyonları
    XGrabKey(display, keys.d_key, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.q_key, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.t_key, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.h_key, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.l_key, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.return_key, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.return_key, MODKEY | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);
    
    // Ses tuşları (modifikatör olmadan)
    XGrabKey(display, keys.volume_raise_key, 0, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.volume_lower_key, 0, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.volume_mute_key, 0, root, True, GrabModeAsync, GrabModeAsync);
    
    // Alt + 1-9 tuşları
    for (int i = XK_1; i <= XK_9; i++) {
        KeyCode keycode = XKeysymToKeycode(display, i);
        XGrabKey(display, keycode, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
        // Alt + Shift + 1-9 için
        XGrabKey(display, keycode, MODKEY | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);
    }

    // Boşlukları aç/kapa
    XGrabKey(display, keys.g_key, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.j_key, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.j_key, MODKEY | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.k_key, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.k_key, MODKEY | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);

    // Sol ok tuşu
    XGrabKey(display, keys.left_key, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.left_key, MODKEY | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);

    // Sağ ok tuşu
    XGrabKey(display, keys.right_key, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.right_key, MODKEY | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);

    // Tab tuşu
    XGrabKey(display, keys.tab_key, MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keys.tab_key, MODKEY | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);
}

// Global değişken olarak ekle (diğer global değişkenlerin yanına)
Cursor normal_cursor;

// Bildirim penceresi için global değişkenler
Window notification_window = None;
int notification_timeout = 1000;  // milisaniye cinsinden (1 saniye)
unsigned long popup_timer = 0;  // Zamanlayıcı için

// Bar için global değişkenler
Window bar_window = None;
int bar_exists = 0;
int effective_screen_y;  // Bar'dan sonra başlayan ekran y koordinatı
int effective_screen_height;  // Bar'dan geriye kalan ekran yüksekliği

// EWMH Atomları için global değişkenler
Atom _NET_WM_WINDOW_TYPE;
Atom _NET_WM_WINDOW_TYPE_DOCK;
Atom _NET_WM_DESKTOP;
Atom _NET_CURRENT_DESKTOP;
Atom _NET_NUMBER_OF_DESKTOPS;
Atom _NET_CLIENT_LIST;
Atom _NET_ACTIVE_WINDOW;
Atom _NET_WM_STRUT_PARTIAL;
Atom _NET_WM_STRUT;
Atom _NET_WM_STATE;
Atom _NET_WM_STATE_DEMANDS_ATTENTION;
Atom _NET_SUPPORTED;

// Pencere tipini kontrol et
int is_bar_window(Window window) {
    XWindowAttributes attrs;
    XGetWindowAttributes(display, window, &attrs);
    
    // _NET_WM_WINDOW_TYPE atomunu al
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;
    Atom window_type_atom = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom dock_atom = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    
    if (XGetWindowProperty(display, window, window_type_atom,
                          0, 1, False, XA_ATOM, &actual_type,
                          &actual_format, &nitems, &bytes_after,
                          &data) == Success && data) {
        Atom type = *(Atom*)data;
        XFree(data);
        if (type == dock_atom) {
            return 1;
        }
    }
    
    // Pencere ismini kontrol et (polybar veya lemonbar için)
    XClassHint class_hint;
    if (XGetClassHint(display, window, &class_hint)) {
        int is_bar = (strcmp(class_hint.res_class, "Polybar") == 0 ||
                     strcmp(class_hint.res_class, "lemonbar") == 0);
        XFree(class_hint.res_name);
        XFree(class_hint.res_class);
        return is_bar;
    }
    
    return 0;
}

// Ekran boyutlarını bar'a göre güncelle
void update_screen_dimensions_with_bar() {
    Screen *screen = DefaultScreenOfDisplay(display);
    screen_width = WidthOfScreen(screen);
    screen_height = HeightOfScreen(screen);
    
    if (bar_exists) {
        if (BAR_POSITION_TOP) {
            effective_screen_y = BAR_HEIGHT;
            effective_screen_height = screen_height - BAR_HEIGHT;
        } else {
            effective_screen_y = 0;
            effective_screen_height = screen_height - BAR_HEIGHT;
        }
    } else {
        effective_screen_y = 0;
        effective_screen_height = screen_height;
    }
    
    // Ana bölge genişliğini yüzdeye göre güncelle
    master_width = (int)((float)screen_width * (master_size_percent / 100.0));
}

// Tiling moduna geç
void toggle_tiling_mode() {
    // Aktif workspace'in modunu değiştir
    workspaces[current_workspace].mode = 
        workspaces[current_workspace].mode == MODE_FLOATING ? MODE_TILING : MODE_FLOATING;
    
    printf("Workspace %d modu değiştirildi: %s\n", 
           current_workspace + 1,
           workspaces[current_workspace].mode == MODE_FLOATING ? "Serbest" : "Döşeli");
    
    // Mevcut workspace'teki pencereleri yeniden düzenle
    rearrange_windows();
}

// Ekran boyutlarını güncelle
void update_screen_dimensions() {
    Screen *screen = DefaultScreenOfDisplay(display);
    screen_width = WidthOfScreen(screen);
    screen_height = HeightOfScreen(screen);
    // Ana bölge genişliğini yüzdeye göre güncelle
    master_width = (int)((float)screen_width * (master_size_percent / 100.0));
}

// Aktif workspace'teki pencereleri düzenle
void rearrange_windows() {
    Workspace *ws = &workspaces[current_workspace];
    
    // Eğer bu workspace serbest modda ise düzenleme yapma
    if (ws->mode == MODE_FLOATING) {
        return;
    }
    
    int window_count = ws->window_count;
    if (window_count == 0) {
        return;
    }
    
    update_screen_dimensions_with_bar();

    // Boşlukları hesapla
    int effective_outer_gap = gaps_enabled ? outer_gap : 0;
    int effective_inner_gap = gaps_enabled ? inner_gap : 0;
    
    // Çalışma alanını hesapla
    int work_x = effective_outer_gap;
    int work_y = effective_screen_y + effective_outer_gap;  // Bar'ı hesaba kat
    int work_width = screen_width - (2 * effective_outer_gap);
    int work_height = effective_screen_height - (2 * effective_outer_gap);  // Bar'ı hesaba kat
    
    if (window_count == 1) {
        // Tek pencere varsa, çalışma alanını kapla
        XMoveResizeWindow(display, ws->windows[0],
                         work_x,
                         work_y,
                         work_width,
                         work_height);
        return;
    }

    // Ana bölge genişliğini yüzdeye göre hesapla
    int master_area_width = (int)((float)work_width * (master_size_percent / 100.0));

    // Ana pencereyi yerleştir
    XMoveResizeWindow(display, ws->windows[0],
                     work_x,
                     work_y,
                     master_area_width - effective_inner_gap,
                     work_height);
    
    // Yığın bölgesini hesapla
    int stack_x = work_x + master_area_width + effective_inner_gap;
    int stack_width = work_width - master_area_width - effective_inner_gap;
    int stack_height = (work_height - ((window_count - 2) * effective_inner_gap)) / (window_count - 1);
    
    // Diğer pencereleri yığında düzenle
    int stack_y = work_y;
    for (int i = 1; i < window_count; i++) {
        XMoveResizeWindow(display, ws->windows[i],
                         stack_x,
                         stack_y,
                         stack_width,
                         stack_height);
        stack_y += stack_height + effective_inner_gap;
    }

    // Değişiklikleri hemen uygula
    XSync(display, False);
}

// Ana bölge genişliğini yüzdesel olarak ayarla
void adjust_master_size(float delta_percent) {
    // Eğer mevcut workspace tiling modunda değilse işlem yapma
    if (workspaces[current_workspace].mode != MODE_TILING) {
        printf("Master boyutu sadece tiling modunda ayarlanabilir\n");
        return;
    }

    // Yeni yüzdeyi hesapla
    master_size_percent += delta_percent;

    // Sınırları kontrol et (%10 ile %90 arası)
    if (master_size_percent < 10.0) {
        master_size_percent = 10.0;
    } else if (master_size_percent > 90.0) {
        master_size_percent = 90.0;
    }

    // Piksel cinsinden genişliği hesapla
    master_width = (int)((float)screen_width * (master_size_percent / 100.0));

    printf("Ana bölge genişliği: %.1f%%\n", master_size_percent);

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
        workspaces[i].mode = MODE_FLOATING;  // Başlangıçta serbest mod
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

    is_switching_workspace = 1;  // Workspace değişimi başladı

    // Mevcut workspace'deki pencereleri gizle
    for (int i = 0; i < workspaces[current_workspace].window_count; i++) {
        Window w = workspaces[current_workspace].windows[i];
        if (w != None) {
            XUnmapWindow(display, w);
        }
    }

    // Yeni workspace'e geç
    current_workspace = new_workspace;

    // Yeni workspace'deki pencereleri göster
    for (int i = 0; i < workspaces[current_workspace].window_count; i++) {
        Window w = workspaces[current_workspace].windows[i];
        if (w != None) {
            XMapWindow(display, w);
        }
    }

    // Workspace değişiklik bildirimini göster
    show_workspace_notification(current_workspace);

    // Yeni workspace'in moduna göre pencereleri düzenle
    if (workspaces[current_workspace].mode == MODE_TILING) {
        rearrange_windows();
    }

    XSync(display, False);

    // EWMH özelliklerini güncelle
    update_workspace_properties();
}

// Pencereyi odakla
void focus_window(Window window) {
    if (window == None || window == root) {
        return;
    }

    // Önceki odaklanmış pencereyi temizle
    if (focused_window != None) {
        XSetWindowBorder(display, focused_window, WINDOW_BORDER_FG);  // Siyah kenarlık
        XSetWindowBorderWidth(display, focused_window, BORDER_WIDTH);
    }

    // Yeni pencereyi odakla
    focused_window = window;
    XSetWindowBorder(display, window, ACTIVE_WINDOW_BORDER_FG);  // Mavi tonunda kenarlık
    XSetWindowBorderWidth(display, window, BORDER_WIDTH);
    XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(display, window);

    // EWMH özelliklerini güncelle
    update_workspace_properties();
}

// Yeni pencere oluşturma isteğini işle
void handle_map_request(XMapRequestEvent *event) {
    // Önce pencerenin bar olup olmadığını kontrol et
    if (is_bar_window(event->window)) {
        // Bar penceresini kaydet ve ekran boyutlarını güncelle
        bar_window = event->window;
        bar_exists = 1;
        update_screen_dimensions_with_bar();
        
        // Bar'ı görünür yap ve yönet
        XMapWindow(display, event->window);
        XSelectInput(display, event->window,
                    StructureNotifyMask | PropertyChangeMask);
        
        // Bar'ı her zaman en üstte tut
        XWindowChanges changes;
        changes.stack_mode = TopIf;
        XConfigureWindow(display, event->window, CWStackMode, &changes);
        
        // Mevcut pencereleri yeni ekran boyutlarına göre düzenle
        rearrange_windows();
        
        printf("Bar penceresi tanındı ve yapılandırıldı\n");
        return;
    }

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
                KeyPressMask);
    
    // Kenarlık kalınlığını ayarla
    XSetWindowBorderWidth(display, event->window, BORDER_WIDTH);
    
    // Eğer bu workspace'teki ilk pencere ise merkeze konumlandır
    if (workspaces[current_workspace].window_count == 0) {
        // Ekran merkezini hesapla
        int center_x = (screen_width - attrs.width) / 2;
        int center_y = (screen_height - attrs.height) / 2;
        
        // Pencereyi merkeze taşı
        XMoveResizeWindow(display, event->window,
                         center_x,
                         center_y,
                         attrs.width,
                         attrs.height);
    } else {
        // Diğer pencereler için normal konumlandırma
        XMoveResizeWindow(display, event->window,
                         attrs.x,
                         attrs.y,
                         attrs.width,
                         attrs.height);
    }
    
    // Pencereyi mevcut workspace'e ekle
    add_window_to_workspace(event->window, current_workspace);
    
    // Döşeli modda pencereleri yeniden düzenle
    if (workspaces[current_workspace].mode == MODE_TILING) {
        rearrange_windows();
    }
    
    // Yeni pencereyi otomatik odakla
    focus_window(event->window);
    
    printf("Yeni pencere workspace %d'e eklendi: %ld\n", current_workspace + 1, event->window);

    // Pencereye workspace özelliğini ata
    long desktop = current_workspace;
    XChangeProperty(display, event->window, _NET_WM_DESKTOP, XA_CARDINAL, 32,
                   PropModeReplace, (unsigned char *)&desktop, 1);

    // EWMH özelliklerini güncelle
    update_workspace_properties();
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
    // Bar penceresi yok edildiyse güncelle
    if (event->window == bar_window) {
        bar_window = None;
        bar_exists = 0;
        update_screen_dimensions_with_bar();
        rearrange_windows();
        printf("Bar penceresi kaldırıldı\n");
        return;
    }
    // Pencereyi mevcut workspace'den kaldır
    remove_window_from_workspace(event->window, current_workspace);
    
    // Pencereler kaldırıldıktan sonra yeniden düzenle
    if (workspaces[current_workspace].mode == MODE_TILING) {
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
    changes.border_width = BORDER_WIDTH;  // Kenarlık kalınlığını sabitle
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
    } else if (is_switching_workspace) {
        // Workspace değişimi sırasında fare hareketi algılandı
        is_switching_workspace = 0;
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

// Pencereyi kapat
void close_window(Window window) {
    if (window == None || window == root) {
        return;
    }
    
    // Pencere mevcut workspace'te mi kontrol et
    int window_in_current_workspace = 0;
    for (int i = 0; i < workspaces[current_workspace].window_count; i++) {
        if (workspaces[current_workspace].windows[i] == window) {
            window_in_current_workspace = 1;
            break;
        }
    }
    
    // Eğer pencere mevcut workspace'te değilse işlem yapma
    if (!window_in_current_workspace) {
        printf("Pencere %ld mevcut workspace'te değil, kapatma işlemi iptal edildi\n", window);
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
    printf("Pencere kapatma isteği gönderildi: %ld (Workspace %d)\n", 
           window, current_workspace + 1);
}

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

     // Pencerenin _NET_WM_DESKTOP özelliğini güncelle
    long desktop = to_ws;
    XChangeProperty(display, window, _NET_WM_DESKTOP, XA_CARDINAL, 32,
                   PropModeReplace, (unsigned char *)&desktop, 1);
        
    // Aktif workspace değiştiyse, pencereyi sakla/göster
    if (current_workspace != to_ws) {
        XUnmapWindow(display, window);
    } else {
        XMapWindow(display, window);
    }
    
    // Her workspace'in kendi moduna göre düzenleme yap
    if (workspaces[from_ws].mode == MODE_TILING) {
        // Geçici olarak workspace'i değiştir ve düzenle
        int temp_ws = current_workspace;
        current_workspace = from_ws;
        rearrange_windows();
        current_workspace = temp_ws;
    }
    
    if (workspaces[to_ws].mode == MODE_TILING && to_ws != from_ws) {
        // Geçici olarak workspace'i değiştir ve düzenle
        int temp_ws = current_workspace;
        current_workspace = to_ws;
        rearrange_windows();
        current_workspace = temp_ws;
    }

    // EWMH özelliklerini güncelle
    update_workspace_properties();
    
    // Pencere listesini güncelle
    Window client_list[MAX_WINDOWS * NUM_WORKSPACES];
    int client_count = 0;
    
    // Tüm workspace'lerdeki pencereleri topla
    for (int i = 0; i < NUM_WORKSPACES; i++) {
        for (int j = 0; j < workspaces[i].window_count; j++) {
            client_list[client_count++] = workspaces[i].windows[j];
        }
    }
    
    // _NET_CLIENT_LIST özelliğini güncelle
    XChangeProperty(display, root, _NET_CLIENT_LIST, XA_WINDOW, 32,
                   PropModeReplace, (unsigned char *)client_list, client_count);
    
    printf("Pencere %ld workspace %d'den %d'e taşındı\n", 
           window, from_ws + 1, to_ws + 1);
}

// Klavye olaylarını işle
void handle_key_press(XKeyEvent *event) {
    KeySym keysym = XkbKeycodeToKeysym(display, event->keycode, 0, 0);
    
    // Alt+Shift kombinasyonlarını kontrol et
    if ((event->state & MODKEY) && (event->state & ShiftMask)) {
        if (keysym >= XK_1 && keysym <= XK_9) {
            // Alt + Shift + 1-9: Aktif pencereyi başka workspace'e taşı
            if (focused_window != None) {
                int target_workspace = keysym - XK_1;
                printf("Alt + Shift + %d tuşuna basıldı\n", target_workspace + 1);
                move_window_to_workspace(focused_window, current_workspace, target_workspace);
            }
        }
        else if (event->keycode == keys.return_key) {
            // Alt + Shift + Enter: Terminal aç
            printf("Terminal açılıyor...\n");
            exec_command(TERMINAL);
        }
        else if (event->keycode == keys.k_key) {
            // Alt + Shift + k: Dış boşlukları artır
            adjust_gaps(5, 0);
        }
        else if (event->keycode == keys.j_key) {
            // Alt + Shift + j: Dış boşlukları azalt
            adjust_gaps(-5, 0);
        }
        else if (event->keycode == keys.q_key) {
            // X oturumunu kapat
            exec_command("pkill X");
        }
    }
    // Sadece Alt tuşu kombinasyonlarını kontrol et
    else if (event->state & MODKEY) {
        if (keysym >= XK_1 && keysym <= XK_9) {
            // Alt + 1-9: Workspace değiştir
            int workspace = keysym - XK_1;
            printf("Alt + %d tuşuna basıldı\n", workspace + 1);
            switch_workspace(workspace);
        }
        else if (event->keycode == keys.d_key) {
            // Alt + d: dmenu çalıştır
            printf("dmenu çalıştırılıyor...\n");
            exec_command(LAUNCHER);
        }
        else if (event->keycode == keys.q_key) {
            // Alt + q: Aktif pencereyi kapat
            if (focused_window != None) {
                // Pencereyi kapat (workspace kontrolü close_window içinde yapılacak)
                close_window(focused_window);
            }
        }
        else if (event->keycode == keys.t_key) {
            // Alt + t: Tiling modunu değiştir
            toggle_tiling_mode();
        }
        else if (event->keycode == keys.h_key) {
            // Alt + h: Ana bölgeyi %1 daralt (sola doğru)
            adjust_master_size(-1.0);
        }
        else if (event->keycode == keys.l_key) {
            // Alt + l: Ana bölgeyi %1 genişlet (sağa doğru)
            adjust_master_size(1.0);
        }
        else if (event->keycode == keys.return_key) {
            // Alt + Enter: Ana pencere ile değiştir
            swap_master();
        }
        else if (event->keycode == keys.g_key) {
            // Alt + g: Boşlukları aç/kapa
            toggle_gaps();
        }
        else if (event->keycode == keys.k_key) {
            // Alt + k: İç boşlukları artır
            adjust_gaps(0, 5);
        }
        else if (event->keycode == keys.j_key) {
            // Alt + j: İç boşlukları azalt
            adjust_gaps(0, -5);
        }
        else if (event->keycode == keys.left_key) {
            // Alt + Sol: Önceki workspace'e git
            int prev_workspace = (current_workspace - 1 + NUM_WORKSPACES) % NUM_WORKSPACES;
            switch_workspace(prev_workspace);
        }
        else if (event->keycode == keys.right_key) {
            // Alt + Sağ: Sonraki workspace'e git
            int next_workspace = (current_workspace + 1) % NUM_WORKSPACES;
            switch_workspace(next_workspace);
        }
        else if (event->keycode == keys.tab_key) {
            // Alt + Tab: Bir sonraki pencereye odaklan
            focus_next_window();
        }
    }
    else if (event->keycode == keys.volume_raise_key) {
        // Ses açma tuşu
        exec_command("amixer set Master 5%+");
    }
    else if (event->keycode == keys.volume_lower_key) {
        // Ses kısma tuşu
        exec_command("amixer set Master 5%-");
    }
    else if (event->keycode == keys.volume_mute_key) {
        // Ses kapatma tuşu
        exec_command("amixer set Master toggle");
    }
}

// Boşlukları aç/kapa
void toggle_gaps() {
    gaps_enabled = !gaps_enabled;
    printf("Boşluklar %s\n", gaps_enabled ? "açıldı" : "kapatıldı");
    rearrange_windows();
}

// Boşluk boyutlarını ayarla
void adjust_gaps(int outer_delta, int inner_delta) {
    if (!gaps_enabled) return;

    // Dış boşlukları ayarla (minimum 0, maksimum 50 piksel)
    outer_gap = outer_gap + outer_delta;
    if (outer_gap < 0) outer_gap = 0;
    if (outer_gap > 50) outer_gap = 50;

    // İç boşlukları ayarla (minimum 0, maksimum 50 piksel)
    inner_gap = inner_gap + inner_delta;
    if (inner_gap < 0) inner_gap = 0;
    if (inner_gap > 50) inner_gap = 50;

    printf("Boşluklar güncellendi - Dış: %d, İç: %d\n", outer_gap, inner_gap);
    rearrange_windows();
}

// Workspace içinde bir sonraki pencereye geç
void focus_next_window() {
    Workspace *ws = &workspaces[current_workspace];
    if (ws->window_count <= 1) return;  // Tek pencere varsa işlem yapma
    
    // Mevcut odaklanmış pencerenin indeksini bul
    int current_index = -1;
    for (int i = 0; i < ws->window_count; i++) {
        if (ws->windows[i] == focused_window) {
            current_index = i;
            break;
        }
    }
    
    // Bir sonraki pencereyi hesapla (döngüsel olarak)
    int next_index = (current_index + 1) % ws->window_count;
    
    // Yeni pencereye odaklan
    focus_window(ws->windows[next_index]);
    
    printf("Pencere odağı değiştirildi: %ld -> %ld\n", 
           ws->windows[current_index], ws->windows[next_index]);
}

// Bildirim penceresini oluştur
void create_notification_window() {
    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    attrs.background_pixel = WS_NOTIFICATION_BG; 
    attrs.border_pixel = WS_NOTIFICATION_BORDER;

    // Ekran ortasında konumlandır
    int width = 100;
    int height = 50;
    int x = (screen_width - width) / 2;
    int y = (screen_height - height) / 2;

    notification_window = XCreateWindow(display, root,
                                     x, y, width, height,
                                     2, // border width
                                     DefaultDepth(display, DefaultScreen(display)),
                                     CopyFromParent,
                                     DefaultVisual(display, DefaultScreen(display)),
                                     CWOverrideRedirect | CWBackPixel | CWBorderPixel,
                                     &attrs);

    XSelectInput(display, notification_window, ExposureMask);
}

// Bildirim penceresini göster
void show_workspace_notification(int workspace_num) {
    if (notification_window == None) {
        create_notification_window();
    }

    // Pencereyi göster
    XMapRaised(display, notification_window);

    // Pencereyi çiz
    XGCValues values;
    GC gc = XCreateGC(display, notification_window, 0, &values);
    XSetForeground(display, gc, WS_NOTIFICATION_FG);

    char text[32];
    snprintf(text, sizeof(text), "%d", workspace_num + 1);

    // Metni ortala
    XFontStruct* font = XLoadQueryFont(display, "fixed");
    if (font) {
        XSetFont(display, gc, font->fid);
        int text_width = XTextWidth(font, text, strlen(text));
        int x = (100 - text_width) / 2;  // 100 = window width
        int y = (50 + (font->ascent - font->descent)) / 2;  // 50 = window height
        XDrawString(display, notification_window, gc, x, y, text, strlen(text));
        XFreeFont(display, font);
    }

    XFreeGC(display, gc);
    XFlush(display);

    // Zamanlayıcı ayarla
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    popup_timer = (current_time.tv_sec * 1000) + (current_time.tv_usec / 1000);
}

// Bildirim penceresini kontrol et
void check_notification_timeout() {
    if (notification_window == None || popup_timer == 0) return;

    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    unsigned long current_ms = (current_time.tv_sec * 1000) + (current_time.tv_usec / 1000);

    if (current_ms - popup_timer >= notification_timeout) {
        XUnmapWindow(display, notification_window);
        popup_timer = 0;
    }
}

void init_atoms() {
    _NET_WM_WINDOW_TYPE = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    _NET_WM_WINDOW_TYPE_DOCK = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    _NET_WM_DESKTOP = XInternAtom(display, "_NET_WM_DESKTOP", False);
    _NET_CURRENT_DESKTOP = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
    _NET_NUMBER_OF_DESKTOPS = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);
    _NET_CLIENT_LIST = XInternAtom(display, "_NET_CLIENT_LIST", False);
    _NET_ACTIVE_WINDOW = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    _NET_WM_STRUT_PARTIAL = XInternAtom(display, "_NET_WM_STRUT_PARTIAL", False);
    _NET_WM_STRUT = XInternAtom(display, "_NET_WM_STRUT", False);
    _NET_WM_STATE = XInternAtom(display, "_NET_WM_STATE", False);
    _NET_WM_STATE_DEMANDS_ATTENTION = XInternAtom(display, "_NET_WM_STATE_DEMANDS_ATTENTION", False);
    _NET_SUPPORTED = XInternAtom(display, "_NET_SUPPORTED", False);

}

void set_supported_hints() {
    Atom supported[] = {
        _NET_WM_STATE,
        _NET_WM_STATE_DEMANDS_ATTENTION,
        _NET_WM_DESKTOP,
        _NET_CURRENT_DESKTOP,
        _NET_NUMBER_OF_DESKTOPS,
        _NET_CLIENT_LIST,
        _NET_ACTIVE_WINDOW,
        _NET_WM_WINDOW_TYPE,
        _NET_WM_WINDOW_TYPE_DOCK
    };
    
    XChangeProperty(display, root, _NET_SUPPORTED, XA_ATOM, 32,
                   PropModeReplace, (unsigned char *)supported,
                   sizeof(supported) / sizeof(Atom));
}

void handle_client_message(XClientMessageEvent *event) {
    if (event->message_type == _NET_CURRENT_DESKTOP) {
        // Workspace değiştirme isteği
        int new_workspace = event->data.l[0];
        if (new_workspace >= 0 && new_workspace < NUM_WORKSPACES) {
            switch_workspace(new_workspace);
        }
    }
    else if (event->message_type == _NET_ACTIVE_WINDOW) {
        // Pencere aktifleştirme isteği
        Window window = event->window;
        // Pencereyi bul ve aktifleştir
        for (int i = 0; i < NUM_WORKSPACES; i++) {
            for (int j = 0; j < workspaces[i].window_count; j++) {
                if (workspaces[i].windows[j] == window) {
                    // Eğer pencere başka bir workspace'te ise, o workspace'e geç
                    if (i != current_workspace) {
                        switch_workspace(i);
                    }
                    focus_window(window);
                    return;
                }
            }
        }
    }
    else if (event->message_type == _NET_WM_STATE) {
        // Pencere durumu değiştirme isteği
        // (şimdilik sadece _NET_WM_STATE_DEMANDS_ATTENTION için)
        if (event->data.l[1] == _NET_WM_STATE_DEMANDS_ATTENTION ||
            event->data.l[2] == _NET_WM_STATE_DEMANDS_ATTENTION) {
            // İsteğe bağlı: Dikkat çekme durumunu işle
        }
    }
}

void update_workspace_properties() {
    // Mevcut workspace'i güncelle
    long data = current_workspace;
    XChangeProperty(display, root, _NET_CURRENT_DESKTOP, XA_CARDINAL, 32,
                   PropModeReplace, (unsigned char *)&data, 1);

    // Toplam workspace sayısını güncelle
    data = NUM_WORKSPACES;
    XChangeProperty(display, root, _NET_NUMBER_OF_DESKTOPS, XA_CARDINAL, 32,
                   PropModeReplace, (unsigned char *)&data, 1);

    // Aktif pencereyi güncelle
    if (focused_window != None) {
        XChangeProperty(display, root, _NET_ACTIVE_WINDOW, XA_WINDOW, 32,
                       PropModeReplace, (unsigned char *)&focused_window, 1);
    }

    // Pencere listesini güncelle
    Window client_list[MAX_WINDOWS * NUM_WORKSPACES];
    int client_count = 0;
    
    // Her workspace'teki pencereleri ekle
    for (int i = 0; i < NUM_WORKSPACES; i++) {
        for (int j = 0; j < workspaces[i].window_count; j++) {
            Window win = workspaces[i].windows[j];
            client_list[client_count++] = win;
            
            // Her pencere için workspace bilgisini güncelle
            long desktop = i;
            XChangeProperty(display, win, _NET_WM_DESKTOP, XA_CARDINAL, 32,
                          PropModeReplace, (unsigned char *)&desktop, 1);
        }
    }
    
    XChangeProperty(display, root, _NET_CLIENT_LIST, XA_WINDOW, 32,
                   PropModeReplace, (unsigned char *)client_list, client_count);
    // Değişiklikleri hemen uygula
    XSync(display, False);
}

void handle_strut_properties(Window window) {
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;
    
    // Önce _NET_WM_STRUT_PARTIAL'ı kontrol et
    if (XGetWindowProperty(display, window, _NET_WM_STRUT_PARTIAL,
                          0, 12, False, XA_CARDINAL, &actual_type,
                          &actual_format, &nitems, &bytes_after,
                          &data) == Success && data) {
        long *struts = (long *)data;
        if (nitems == 12) {
            // Strut değerlerini işle
            if (struts[2] > 0) {  // Top strut
                effective_screen_y = struts[2];
                effective_screen_height = screen_height - struts[2];
            } else if (struts[3] > 0) {  // Bottom strut
                effective_screen_height = screen_height - struts[3];
            }
        }
        XFree(data);
    }
    // Sonra _NET_WM_STRUT'u kontrol et
    else if (XGetWindowProperty(display, window, _NET_WM_STRUT,
                               0, 4, False, XA_CARDINAL, &actual_type,
                               &actual_format, &nitems, &bytes_after,
                               &data) == Success && data) {
        long *struts = (long *)data;
        if (nitems == 4) {
            if (struts[2] > 0) {  // Top strut
                effective_screen_y = struts[2];
                effective_screen_height = screen_height - struts[2];
            } else if (struts[3] > 0) {  // Bottom strut
                effective_screen_height = screen_height - struts[3];
            }
        }
        XFree(data);
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
    
    // Ekran boyutlarını başlat
    update_screen_dimensions();
    
    // Tuş kodlarını başlat - YENİ
    init_keybindings();

    // Root pencere için olay maskesini güncelle
    XSelectInput(display, root,
                SubstructureRedirectMask |
                SubstructureNotifyMask |
                ButtonPressMask |
                ButtonReleaseMask |
                PointerMotionMask |
                PropertyChangeMask | 
                KeyPressMask);

    // Klavye olaylarını root pencereye yönlendir
    grab_keys();  // YENİ - Önceki tüm XGrabKey çağrıları yerine

    // Normal fare işaretçisini oluştur
    normal_cursor = XCreateFontCursor(display, XC_left_ptr);
    XDefineCursor(display, root, normal_cursor);

    XSetErrorHandler(error_handler);

    printf("Pencere yöneticisi başlatıldı...\n");
    printf("Alt + 1-9: Workspace değiştir\n");
    printf("Alt + Shift + 1-9: Aktif pencereyi belirtilen workspace'e taşı\n");
    printf("Alt + d: dmenu çalıştır\n");
    printf("Alt + q: Aktif pencereyi kapat\n");
    printf("Alt + t: Tiling/Floating mod değiştir\n");
    printf("Alt + l: Ana bölgeyi %%1 genişlet (sağa doğru)\n");
    printf("Alt + h: Ana bölgeyi %%1 daralt (sola doğru)\n");
    printf("Alt + Enter: Ana pencere ile değiştir\n");
    printf("Alt + g: Boşlukları aç/kapa\n");
    printf("Alt + j/k: İç boşlukları azalt/artır\n");
    printf("Alt + Shift + j/k: Dış boşlukları azalt/artır\n");
    printf("Alt + Sol/Sağ: Önceki/Sonraki workspace'e geç\n");
    printf("Alt + Tab: Workspace içinde pencereler arası geçiş yap\n");
    printf("Alt + Shift + q: X oturmunu kapatır\n");
    printf("Alt + Shift + Enter: Terminal açar\n");

    // EWMH atomlarını başlat
    init_atoms();

    // Desteklenen özellikleri bildir
    set_supported_hints();
    
    // İlk workspace özelliklerini ayarla
    update_workspace_properties();

    // Ana döngü
    XEvent event;
    while (1) {
        // Bildirim zamanlayıcısını kontrol et
        check_notification_timeout();

        while (XPending(display)) {
            XNextEvent(display, &event);
            
            switch (event.type) {
                 case ClientMessage:
                    handle_client_message(&event.xclient);
                    break;
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
                    if (event.xcrossing.mode == NotifyNormal && !is_switching_workspace) {
                        focus_window(event.xcrossing.window);
                    }
                    break;
                case Expose:
                    if (event.xexpose.window == notification_window) {
                        // Bildirim penceresini yeniden çiz
                        show_workspace_notification(current_workspace);
                    }
                    break;
            }
        }
        
        usleep(1000);  // CPU kullanımını azaltmak için kısa bekleme
    }

    // Program sonunda temizlik
    XUngrabKey(display, AnyKey, AnyModifier, root);
    XFreeCursor(display, normal_cursor);
    XCloseDisplay(display);
    return 0;
}


