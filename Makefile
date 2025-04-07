PREFIX?=/usr/X11R6
CFLAGS?=-Os -pedantic -Wall
BINDIR?=/usr/local/bin

all:
	$(CC) $(CFLAGS) -I$(PREFIX)/include main.c -L$(PREFIX)/lib -lX11 -o mwm

install: mwm
	install -Dm755 mwm $(BINDIR)/mwm

uninstall:
	rm -f $(BINDIR)/mwm

clean:
	rm -f mwm