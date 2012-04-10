
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/Xft/Xft.h>


#define LENGTH(A) (sizeof(A) / sizeof(A[0]))
#define err(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define die(fmt, ...) do { fprintf(stderr, fmt, ##__VA_ARGS__); exit(1); } while(0)
#define CENTER(A, B) (( (A)/2 ) - ( (B)/2 ))
#define CLEANMASK(mask) (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

struct
config {
	unsigned int mod;
	KeySym key;
	char *text;
	void (*disp)(struct config *, char *);
	char *cmd;
};

struct
font {
	XftFont *xfont;
	int h;
};

void text_with_bar(struct config *c, char *in);
void text_with_text(struct config *c, char *in);

#include "config.h"

Display *dpy;
Window win;
Window root;
int screen;
Visual *visual;
Colormap cmap;
GC gc;
struct font fontbig;
struct font fontsmall;
XftFont *xfont;
int fonth;
Atom NetWMWindowOpacity;
unsigned int numlockmask = 0;
int wx, wy, sw, sh;

unsigned long fgcol;
unsigned long bgcol;

Bool is_mapped = False;


int
handle_xerror(Display * display, XErrorEvent * e) {
	fprintf(stderr, "%d  XError caught\n", e->error_code);
	return 0;
}

void
setup_font(struct font *font, char *fontstr) {
	if(!(font->xfont = XftFontOpenXlfd(dpy, screen, fontstr))) {
		err("WARNING: Could not setup font: '%s'. Falling back to 'fixed'\n", fontstr);
		if(!(font->xfont = XftFontOpenXlfd(dpy, screen, "fixed")))
			die("ERROR: Could not setup font\n");
	}

	font->h = font->xfont->ascent + font->xfont->descent;
}

void
draw_text(struct font *font, char *str, int x, int y) {
	XftColor color;
	XftDraw *draw;

	color.color.red = (fgcol >> 16) * 0x101;
	color.color.green = (fgcol >> 8) * 0x101;
	color.color.blue = fgcol * 0x101;
	color.color.alpha = 0xFFFF;
	color.pixel = 0xFFFFFF00;

	XftColorAllocValue(dpy, visual, cmap, &color.color, &color);
	draw = XftDrawCreate(dpy, win, visual, cmap);

	XftDrawStringUtf8(draw, &color, font->xfont, x, y, (XftChar8 *)str, strlen(str));
}

int
text_width(struct font *font, char *str) {
	XGlyphInfo ext;

	XftTextExtentsUtf8(dpy, font->xfont, (XftChar8 *)str, strlen(str), &ext);
	return ext.xOff;
}

#define MAX(A, B) ((A) > (B) ? (A) : (B))

void 
text_with_text(struct config *c, char *in) {
	int nww, tws, twb;

	tws = text_width(&fontsmall, in);
	twb = text_width(&fontbig, c->text);
	nww = MAX(ww, MAX(tws, twb) + 20);

	XMoveResizeWindow(dpy, win, CENTER(sw, nww), wy, nww, wh);

	/* Clear window */
	XSetForeground(dpy, gc, bgcol);
	XFillRectangle(dpy, win, gc, 0, 0, nww, wh);

	XSetForeground(dpy, gc, fgcol);
	draw_text(&fontbig, c->text, CENTER(nww, text_width(&fontbig, c->text)),
			CENTER(wh/2, fontbig.h)+fontbig.h);
	draw_text(&fontsmall, in, CENTER(nww, text_width(&fontsmall, in)), (wh/2)+fontsmall.h);

	XSync(dpy, False);
}

void 
text_with_bar(struct config *c, char *in) {
	int nww, twb;

	twb = text_width(&fontbig, c->text);
	nww = MAX(ww, twb + 30);

	XMoveResizeWindow(dpy, win, CENTER(sw, nww), wy, nww, wh);

	XRectangle r = { CENTER(nww, barw), /*CENTER(wh/2, barh)+*/(wh/2), barw, barh };

	/* Clear window */
	XSetForeground(dpy, gc, bgcol);
	XFillRectangle(dpy, win, gc, 0, 0, nww, wh);

	XSetForeground(dpy, gc, fgcol);
	draw_text(&fontbig, c->text, CENTER(nww, text_width(&fontbig, c->text)),
			CENTER(wh/2, fontbig.h)+fontbig.h);
//	printf("font height: %i\n", fonth);

	/* border */
	XDrawRectangles(dpy, win, gc, &r, 1);

	/* and bar */
	r.width = (float)atoi(in)/100.0*(float)barw;
	XFillRectangles(dpy, win, gc, &r, 1);

	XSync(dpy, False);
	//puts(in);
}

void
grabkey(unsigned int modmask, KeySym key) {
	XGrabKey(dpy, XKeysymToKeycode(dpy, key), modmask, root, True,
		GrabModeAsync, GrabModeAsync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, key), LockMask | modmask, root,
		True, GrabModeAsync, GrabModeAsync);

	if(numlockmask) {
		XGrabKey(dpy, XKeysymToKeycode(dpy, key), modmask | numlockmask, root, True,
			GrabModeAsync, GrabModeAsync);
		XGrabKey(dpy, XKeysymToKeycode(dpy, key), LockMask | modmask | numlockmask, root,
			True, GrabModeAsync, GrabModeAsync);
	}
};

unsigned long
getcolor(char *colstr) {
	XColor color;

	if(!XAllocNamedColor(dpy, cmap, colstr, &color, &color))
		die("error, cannot allocate color '%s'\n", colstr);
	return color.pixel;
}

void
sigalrm(int i) {
	signal(SIGALRM, sigalrm);
	XUnmapWindow(dpy, win);
	XFlush(dpy);
	is_mapped = False;
	//puts("Caught sigalrm");
}

void
setup() {
	int i, j;

	XSetWindowAttributes wattr;
	XModifierKeymap *modmap;

	XSetErrorHandler(handle_xerror);

	dpy = XOpenDisplay(NULL);
	root = DefaultRootWindow(dpy);
	screen = DefaultScreen(dpy);
	visual = DefaultVisual(dpy, screen);
	cmap = DefaultColormap(dpy, screen);
	gc = DefaultGC(dpy, screen);

	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	wx = CENTER(sw, ww);
	wy = CENTER(sh, wh);

	wattr.override_redirect = True;
	wattr.background_pixel = bgcol; 
	wattr.border_pixel = fgcol;
	win = XCreateWindow(dpy, root, wx, wy, ww, wh, bw,
			CopyFromParent, InputOutput, CopyFromParent,
			CWOverrideRedirect | CWBackPixel | CWBorderPixel, &wattr);

	/* Grab keys */
	/* modifier stuff taken from evilwm */
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++) {
		for (j = 0; j < modmap->max_keypermod; j++) {
			if (modmap->modifiermap[i * modmap->max_keypermod + j] ==
			XKeysymToKeycode(dpy, XK_Num_Lock)) {
				numlockmask = (1 << i);
			}
		}
	}
	for(i=0; i < LENGTH(conf); i++)
		grabkey(conf[i].mod, conf[i].key);

	/* behöver inte denna va? */
	XSelectInput(dpy, root, KeyPressMask | SubstructureNotifyMask);

	setup_font(&fontbig, fontstrbig);
	setup_font(&fontsmall, fontstrsmall);

	fgcol = getcolor(fgcolor);
	bgcol = getcolor(bgcolor);

	NetWMWindowOpacity = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);
	if(opacity > 1.0 || opacity < 0.0)
		opacity = 0.0;
	unsigned long real_opacity[] = { opacity * 0xffffffff };
	XChangeProperty(dpy, win, NetWMWindowOpacity, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)real_opacity, 1);

	signal(SIGALRM, sigalrm);
}

void
start_timer() {
	struct itimerval t;

	memset(&t, 0, sizeof t);
	t.it_value.tv_sec = 2;
	if(setitimer(ITIMER_REAL, &t, 0) == -1)
		die("Could not set timer: %s\n", strerror(errno));
}

void
run() {
	XEvent ev;
	KeySym keysym;
	char buf[256];
	int i, len;
	FILE *f;

	XSync(dpy, False);
	while(!XNextEvent(dpy, &ev)) {
		//puts("got event");
		if(ev.type == KeyPress) {
			//puts("\tkeypress!");
			keysym = XKeycodeToKeysym(dpy, (KeyCode)ev.xkey.keycode, 0);
			for(i=0; i < LENGTH(conf); i++) {
				if(conf[i].key == keysym &&
						CLEANMASK(conf[i].mod) == CLEANMASK(ev.xkey.state)) {
					//printf("%i\n", (int)keysym);
					//puts(conf[i].cmd);
					f = popen(conf[i].cmd, "r");
					if(!f) {
						perror(conf[i].cmd);
						break;
					}

					len = fread(buf, 1, sizeof(buf)-1, f);
					buf[len - (buf[len-1] == '\n' ? 1 : 0)] = '\0';
					fclose(f);

					if(!is_mapped) {
						XMapRaised(dpy, win);
						while (1) {
							XMaskEvent(dpy, SubstructureNotifyMask, &ev);
							if ((ev.type == CreateNotify || ev.type == MapNotify)
								&& ev.xcreatewindow.window == win)
							{
								break;
							}
						}
						is_mapped = True;
					}
					start_timer();
					conf[i].disp(&conf[i], &buf[0]);
				}
			}
		}
	}
}

int
main() {
	setup();
	run();
	return 0;
}

