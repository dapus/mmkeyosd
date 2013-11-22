
include config.mk

CC=gcc
LD=gcc
CFLAGS=-g -Wall -DVERSION=\"${VERSION}\" `pkg-config --cflags xft`
OBJ=mmkeyosd.o config.o
LIBS=-lX11 `pkg-config --libs xft` -lXinerama

all: mmkeyosd

mmkeyosd: $(OBJ)
	@echo LD -o $@
	@$(LD) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

%.o: %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c $< -o $@

${OBJ}: config.h config.mk

install: mmkeyosd
	@echo installing to $(DESTDIR)$(PREFIX)/bin...
	@mkdir -p $(DESTDIR)$(PREFIX)/bin
	@install -m755 mmkeyosd $(DESTDIR)$(PREFIX)/bin
	@mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	@install -m644 mmkeyosd.1 $(DESTDIR)$(MANPREFIX)/man1/


clean:
	rm -f $(OBJ) mmkeyosd

