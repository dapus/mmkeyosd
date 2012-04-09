
include config.mk

CC=gcc
LD=gcc
CFLAGS=-g -Wall `pkg-config --cflags xft`
OBJ=mmkeyosd.o
LIBS=-lX11 `pkg-config --libs xft`

${OBJ}: config.h config.mk

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

all: mmkeyosd

mmkeyosd: $(OBJ)
	@echo LD -o $@
	@$(LD) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

%.o: %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c $< -o $@

install: mmkeyosd
	@echo installing to $(DESTDIR)$(PREFIX)/bin...
	@mkdir -p $(DESTDIR)$(PREFIX)/bin
	@install mmkeyosd $(DESTDIR)$(PREFIX)/bin

clean:
	rm -f $(OBJ) mmkeyosd

