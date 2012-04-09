
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/Xft/Xft.h>


#define LENGTH(A) (sizeof(A) / sizeof(A[0]))
#define err(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define die(fmt, ...) do { fprintf(stderr, fmt, ##__VA_ARGS__); exit(1); } while(0)
#define CENTER(A, B) (( (A)/2 ) - ( (B)/2 ))

struct
config {
	KeySym key;
	char *text;
	void (*disp)(struct config *, char *);
	char *cmd;
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
XftFont *xfont;
int fonth;

unsigned long fgcol;
unsigned long bgcol;

Bool is_mapped = False;


int
handle_xerror(Display * display, XErrorEvent * e) {
	fprintf(stderr, "%d  XError caught\n", e->error_code);
	return 0;
}

void
setup_font() {
	if(!(xfont = XftFontOpenXlfd(dpy, screen, fontstr))) {
		err("WARNING: Could not setup font: '%s'. Falling back to 'fixed'\n", fontstr);
		if(!(xfont = XftFontOpenXlfd(dpy, screen, "fixed")))
			die("ERROR: Could not setup font\n");
	}

	fonth = xfont->ascent + xfont->descent;
}

void
draw_text(char *str, int x, int y) {
	XftColor color;
	XftDraw *draw;

	color.color.red = (fgcol >> 16) * 0x101;
	color.color.green = (fgcol >> 8) * 0x101;
	color.color.blue = fgcol * 0x101;
	color.color.alpha = 0xFFFF;
	color.pixel = 0xFFFFFF00;

	XftColorAllocValue(dpy, visual, cmap, &color.color, &color);
	draw = XftDrawCreate(dpy, win, visual, cmap);

	XftDrawStringUtf8(draw, &color, xfont, x, y, (XftChar8 *)str, strlen(str));
}

int
text_width(char *str) {
	XGlyphInfo ext;

	XftTextExtentsUtf8(dpy, xfont, (XftChar8 *)str, strlen(str), &ext);
	return ext.xOff;
}

void 
text_with_text(struct config *c, char *in) {
	/* Clear window */
	XSetForeground(dpy, gc, bgcol);
	XFillRectangle(dpy, win, gc, 0, 0, ww, wh);

	XSetForeground(dpy, gc, fgcol);
	draw_text(c->text, CENTER(ww, text_width(c->text)), CENTER(wh/2, fonth)+fonth);
	draw_text(in, CENTER(ww, text_width(in)), (wh/2)+fonth);

	XSync(dpy, False);
}

void 
text_with_bar(struct config *c, char *in) {
	XRectangle r = { CENTER(ww, barw), /*CENTER(wh/2, barh)+*/(wh/2), barw, barh };

	/* Clear window */
	XSetForeground(dpy, gc, bgcol);
	XFillRectangle(dpy, win, gc, 0, 0, ww, wh);

	XSetForeground(dpy, gc, fgcol);
	draw_text(c->text, CENTER(ww, text_width(c->text)), CENTER(wh/2, fonth)+fonth);
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
grabkey(KeySym key) {
	int modmask = 0;
	XGrabKey(dpy, XKeysymToKeycode(dpy, key), modmask, root, True,
		GrabModeAsync, GrabModeAsync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, key), LockMask | modmask, root,
		True, GrabModeAsync, GrabModeAsync);
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
	int i, wx, wy;
	XSetWindowAttributes wattr;

	XSetErrorHandler(handle_xerror);

	dpy = XOpenDisplay(NULL);
	root = DefaultRootWindow(dpy);
	screen = DefaultScreen(dpy);
	visual = DefaultVisual(dpy, screen);
	cmap = DefaultColormap(dpy, screen);
	gc = DefaultGC(dpy, screen);

	wx = CENTER(DisplayWidth(dpy, screen), ww);
	wy = CENTER(DisplayHeight(dpy, screen), wh);

	wattr.override_redirect = True;
	wattr.background_pixel = bgcol; 
	wattr.border_pixel = fgcol;
	win = XCreateWindow(dpy, root, wx, wy, ww, wh, bw,
			CopyFromParent, InputOutput, CopyFromParent,
			CWOverrideRedirect | CWBackPixel | CWBorderPixel, &wattr);

	for(i=0; i < LENGTH(conf); i++)
		grabkey(conf[i].key);

	/* behöver inte denna va? */
	XSelectInput(dpy, root, KeyPressMask | SubstructureNotifyMask);

	setup_font();

	fgcol = getcolor(fgcolor);
	bgcol = getcolor(bgcolor);

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
				if(conf[i].key == keysym) {
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

