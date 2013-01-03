#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>

#include "mmkeyosd.h"
#include "util.h"
#include "config.h"

typedef struct String {
	int size, len;
	char *str;
} String;


struct modtable {
	char *name;
	unsigned int mod;
};
struct modtable
modtable[] = {
	{"Control",       ControlMask},
	{"Super",         Mod4Mask},
	{"Alt",           Mod1Mask},
};

/* some dynamic string macros */
#define string_append_ch(string, add) \
	do { \
		while(string.len + 1 + 1 > string.size) \
			string.str = realloc(string.str, (string.size *= 2)); \
		*(string.str + string.len) = add; \
		*(string.str + string.len + 1) = '\0'; \
		string.len += 1; \
	} while(0);

#define string_append(string, add) \
	do { \
		int l; \
		l = strlen(add); \
		while(string.len + l + 1 > string.size) \
			string.str = realloc(string.str, (string.size *= 2)); \
		strcpy(string.str + string.len, add); \
		string.len += l; \
	} while(0);

#define string_clear(string) \
	do { \
		string.str[0] = '\0'; \
		string.len = 0; \
	} while(0);

#define string_init(string) \
	do { \
		string.str = xmalloc(128); \
		*string.str = '\0'; \
		string.size = 128; \
		string.len = 0; \
	} while(0);

#define string_free(string) free(string.str)

/* skip whitespace */
#define skipws(str) \
	for(; *str != '\0' && (*str == ' ' || *str == '\t'); str++);

#define skipwsr(str, start) \
	for(; str != start && (*str == ' ' || *str == '\t'); str--);

void *
xrealloc(void *old, size_t n) {
	void *data;

	if(!(data = realloc(old, n)))
		die("realloc: %s\n", strerror(errno));

	return data;
}

void *
xmalloc(size_t n) {
	void *data;

	if(!(data = malloc(n)))
		die("malloc: %s\n", strerror(errno));

	return data;
}

void
config_add(struct config **list, unsigned int mod, KeySym key, const char *text,
	void (*disp)(struct config *, char *, int), const char *cmd) {
	struct config *new;

	new = xmalloc(sizeof (struct config));
	new->mod = mod;
	new->key = key;
	new->text = strdup(text);
	new->disp = disp;
	new->cmd = strdup(cmd);
	new->prev = new->next = NULL;

	LIST_APPEND(*list, new);
}

KeySym
keyfromstr(const char *str) {
	KeySym k;
	char xf86str[64] = {"XF86"};

	k = XStringToKeysym(str);
	/* Some keys are prefixed with XF86 (specifically those
	 * defined in XF86keysym.h) */
	if(k == NoSymbol) {
		strncpy(&xf86str[4], str, sizeof xf86str - 4);
		xf86str[63] = '\0';
		k = XStringToKeysym(xf86str);
	}

	return k;
}

unsigned int
modfromstr(const char *str) {
	int i;
	unsigned int mod = 0;

	for(i=0; i < LENGTH(modtable); i++) {
		if(strstr(str, modtable[i].name))
			mod |= modtable[i].mod;
	}

	return mod;
}

void 
(*dispfuncfromstr(const char *str))(struct config *, char *, int) {
	if(strcmp(str, "text_with_text") == 0) {
		return text_with_text;
	}
	if(strcmp(str, "text_with_bar") == 0) {
		return text_with_bar;
	}
	return NULL;
}

char *
read_line(char *buf, size_t size, FILE *f, const char *filename) {
	int len;

	if(!fgets(buf, size, f)) {
		if(ferror(f))
			die("Error while reading %s: %s\n", filename, strerror(errno));
		else if(feof(f))
			return NULL;
	}

	len = strlen(buf);

	/* remove newline */
	if(buf[len-1] == '\n')
		buf[--len] = '\0';

	/* skip comments and empty lines */
	if(!len || buf[0] == '#')
		return NULL;

	return buf;
}

char *
nextword(char *start, char **end) {
	char *e;

	skipws(start);
	for(e=start; *e != '\0'; e++) {
		if(*e == ' ' || *e == '\t')
			break;
	}

	*e = '\0';
	if(end)
		*end = e;

	return start;
}

char *
nextquoted(char *start, char **end) {
	char *e;

	skipws(start);
	if(*start != '"')
		return NULL;

	for(e=++start; e != '\0'; e++) {
		if(*e == '"')
			break;
	}
	if(*e == '\0')
		return NULL;

	*e = '\0';
	*end = e;

	return start;
}

struct config *
config_read(const char *file) {
	FILE *f;
	String line;
	struct config *config = NULL;
	char buf[1024];
	int lc;
	char *lineend, *end, *keystr, *text, *cmd, *dispstr, *s, *modstr;
	KeySym k;
	void (*disp)(struct config *, char *, int);

	string_init(line);

	if(!(f = fopen(file, "r"))) {
		err("%s: fopen: %s\n", file, strerror(errno));
		return NULL;
	}

	for(lc=1;;lc++) {
		if(!read_line(buf, sizeof buf, f, file)) {
			if(feof(f))
				break;
			continue;
		}

		string_append(line, buf);

		/* check if we read the whole line */
		if(line.str[line.len-1] == '\\') {
			line.str[--line.len] = '\0';

			if(!feof(f))
				continue;
		}

		lineend = &line.str[line.len];

		/* Get modifiers and key */
		modstr = nextword(line.str, &end);
		if(end == lineend)
			goto syntax;

		s = strchr(modstr, '+');
		if(s) {
			*s = '\0';
			keystr = s+1;
		} else {
			keystr = modstr;
			modstr = NULL;
		}

		/* Get display function */
		dispstr = nextword(end+1, &end);
		if(end == lineend)
			goto syntax;

		/* Get text */
		text = nextquoted(end+1, &end);
		if(text == NULL)
			goto syntax;

		/* Get command */
		cmd = end+1;
		skipws(cmd);
		if(lineend-cmd < 1)
			goto syntax;


		if((k = keyfromstr(keystr)) == NoSymbol)
			die("%s is not a valid hotkey\n", keystr);

		if(!(disp = dispfuncfromstr(dispstr)))
			die("%s is not a valid display function\n", dispstr);

//		printf("'%s', '%s', '%s', '%s', '%s', %i\n", modstr, keystr, dispstr, text, cmd, k); 
		config_add(&config, modstr ? modfromstr(modstr) : 0, k, text, disp, cmd);

		string_clear(line);
	}


	fclose(f);
	string_free(line);
	return config;
syntax:
	die("Error on line %i in %s\n", lc, file);
	return NULL;
}

void
settings_add(struct settings **list, const char *key, const char *value) {
	struct settings *new;

	new = xmalloc(sizeof (struct settings));
	new->key = strdup(key);
	new->value = strdup(value);
	new->prev = new->next = NULL;

	LIST_APPEND(*list, new);
}

struct settings *
settings_read(const char *file) {
	FILE *f;
	struct settings *settings= NULL;
	char buf[1024];
	int lc;
	char *key, *value, *end;

	if(!(f = fopen(file, "r"))) {
		err("%s: fopen: %s\n", file, strerror(errno));
		return NULL;
	}

	for(lc=1;;lc++) {
		if(!read_line(buf, sizeof buf, f, file)) {
			if(feof(f))
				break;
			continue;
		}

		key = buf;
		end = strchr(key, '=');
		if(!end || end == key)
			goto syntax;

		value = end+1;

		end--;
		skipwsr(end, key);
		*++end = '\0';

		skipws(value);
		
//		printf("'%s' '%s'\n", key, value);
		settings_add(&settings, key, value);
	}

	return settings;

syntax:
	die("Error on line %i in %s\n", lc, file);
	return NULL;
}

int
settings_find_int(struct settings *list, const char *key, int def) {
	struct settings *set;

	LIST_FOREACH(list, set) {
		if(strcmp(set->key, key) == 0)
			return atoi(set->value);
	}
	return def;
}

double
settings_find_double(struct settings *list, const char *key, double def) {
	struct settings *set;

	LIST_FOREACH(list, set) {
		if(strcmp(set->key, key) == 0)
			return atof(set->value);
	}
	return def;
}

char *
settings_find_str(struct settings *list, const char *key, char *def) {
	struct settings *set;

	LIST_FOREACH(list, set) {
		if(strcmp(set->key, key) == 0)
			return set->value;
	}
	return def;
}

