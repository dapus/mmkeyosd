#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <X11/Xlib.h>
struct
config {
	unsigned int mod;
	KeySym key;
	char *text;
	void (*disp)(struct config *, char *, int error);
	char *cmd;
	struct config *prev, *next;
};

struct
settings {
	char *key;
	char *value;
	struct settings *prev, *next;
};


struct config *config_read(const char *file);
struct settings *settings_read(const char *file);

int settings_find_int(struct settings *list, const char *key, int def);
double settings_find_double(struct settings *list, const char *key, double def);
char *settings_find_str(struct settings *list, const char *key, char *def);

#endif
