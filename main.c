#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

Display *display;
Window root;

int main() {
    // X sunucusuna bağlan
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "X sunucusuna bağlanılamadı.\n");
        return 1;
    }

    // Root pencereyi al
    root = DefaultRootWindow(display);

    // Pencere yöneticisi olarak kendimizi belirt
    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask);

    // Ana döngü
    XEvent event;
    while (1) {
        XNextEvent(display, &event);
        // Event işleme kodları buraya gelecek
    }

    // Temizlik
    XCloseDisplay(display);
    return 0;
}
