#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>

#include "mmkeyosd.h"
#include "util.h"
#include "config.h"

#include "keys.h"

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
	int i;
	KeySym k;

	k = XStringToKeysym(str);
	if(k != NoSymbol)
		return k;

	for(i=0; i < LENGTH(keytable); i++) {
		if(strcmp(str, keytable[i].name) == 0)
			return keytable[i].key;
	}

	return NoSymbol;
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

struct config *
config_read(const char *file) {
	FILE *f;
	String line;
	struct config *config = NULL;
	char buf[1024];
	int lc, len;
	char *lineend, *start, *end, *keystr, *text, *cmd, *dispstr, *s, *modstr;
	KeySym k;
	void (*disp)(struct config *, char *, int);

	string_init(line);

	if(!(f = fopen(file, "r"))) {
		err("%s: fopen: %s\n", file, strerror(errno));
		return NULL;
	}

	for(lc=0;;lc++) {
		memset(buf, 0, sizeof buf);
		if(!fgets(buf, sizeof buf, f)) {
			if(ferror(f))
				die("Error while reading %s: %s\n", file, strerror(errno));
			else if(feof(f))
				break;
		}

		string_append(line, buf);

		/* check if we read the whole line */
		len = strlen(buf);
		if(buf[len-1] != '\n' || buf[len-2] == '\\') {
			/* remove backslash */
			if(line.str[line.len-2] == '\\') {
				line.str[line.len-2] = '\0';
				line.len-=2;
			}
			if(!feof(f))
				continue;
		}
		/* remove newline */
		if(line.str[line.len-1] == '\n') {
			line.str[line.len-1] = '\0';
			line.len--;
		}
		lineend = &line.str[line.len];

		/* skip comments and empty lines */
		if(!line.len || line.str[0] == '#')
			goto end;

		start = line.str;
		skipws(start);
		for(end=start; end != lineend; end++) {
			if(*end == ' ' || *end == '\t')
				break;
		}
		if(end == lineend)
			goto syntax;
		*end = '\0';
		modstr = start;
		s = strchr(start, '+');
		if(s) {
			*s = '\0';
			keystr = s+1;
		} else {
			keystr = start;
			modstr = NULL;
		}

		start = end+1;
		skipws(start);
		for(end=start; end != lineend; end++) {
			if(*end == ' ' || *end == '\t')
				break;
		}
		if(end == lineend)
			goto syntax;
		*end = '\0';
		dispstr = start;

		start = end+1;
		skipws(start);
		if(*start != '"')
			goto syntax;
		start++;
		for(end=start; end != lineend; end++) {
			if(*end == '"')
				break;
		}
		if(end == lineend)
			goto syntax;
		*end = '\0';
		text = start;

		start = end+1;
		skipws(start);
		cmd = start;
		if(lineend-start < 1)
			goto syntax;

		k = keyfromstr(keystr);
		if(k == NoSymbol)
			die("%s is not a valid hotkey\n", keystr);

		disp = dispfuncfromstr(dispstr);
		if(!disp)
			die("%s is not a valid display function\n", dispstr);

//		printf("'%s', '%s', '%s', '%s', '%s', %i\n", keystr, modstr, text, dispstr, cmd, k); 
		config_add(&config, modstr ? modfromstr(modstr) : 0, k, text, disp, cmd);
end:
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
	char *start, *end, *sstart;

	if(!(f = fopen(file, "r"))) {
		err("%s: fopen: %s\n", file, strerror(errno));
		return NULL;
	}

	for(lc=0;;lc++) {
		memset(buf, 0, sizeof buf);
		if(!fgets(buf, sizeof buf, f)) {
			if(ferror(f))
				die("Error while reading %s: %s\n", file, strerror(errno));
			else if(feof(f))
				break;
		}

		/* skip comments and empty lines */
		if(!strlen(buf) || buf[0] == '#')
			continue;

		/* remove newline */
		if(buf[strlen(buf)-1] == '\n')
			buf[strlen(buf)-1] = '\0';

		start = buf;
		end = strchr(start, '=');
		if(!end || end == start)
			goto syntax;

		sstart = end+1;
		end--;
		skipwsr(end, start);
		end++;
		*end = '\0';

		skipws(sstart);
		
		/* printf("'%s' '%s'\n", start, sstart); */
		settings_add(&settings, start, sstart);
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

