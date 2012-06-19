
include config.mk

CC=gcc
LD=gcc
CFLAGS=-g -Wall `pkg-config --cflags xft`
OBJ=mmkeyosd.o config.o
LIBS=-lX11 `pkg-config --libs xft`

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
	@install mmkeyosd $(DESTDIR)$(PREFIX)/bin

clean:
	rm -f $(OBJ) mmkeyosd

