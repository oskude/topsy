PROG       = topsy
MYCFLAGS   = `pkg-config --cflags xcb xcb-ewmh xcb-icccm cairo`
MYLDFLAGS  = `pkg-config --libs xcb xcb-ewmh xcb-icccm cairo`
PREFIX    ?= /usr/local

.PHONY: all clean install archlinux

all: $(PROG)

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) ${MYCFLAGS} $< -o $@

%: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) ${MYCFLAGS} ${MYLDFLAGS} $^ -o $@

clean:
	rm -f $(PROG) *.o
	rm -rf distro/archlinux/{pkg,src,$(PROG),*.pkg.*}

install: $(PROG)
	install -Dm755 $(PROG) $(DESTDIR)$(PREFIX)/bin/$(PROG)

archlinux:
	cd distro/archlinux;\
		makepkg -f
