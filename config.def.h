
char *fontstrbig = "-*-dejavu sans mono-medium-r-*-*-20-*-*-*-*-*-*-*";
char *fontstrsmall = "-*-dejavu sans mono-medium-r-*-*-13-*-*-*-*-*-*-*";
char *fgcolor = "white";
char *bgcolor = "black";
int bw = 0; /* border width */
int ww = 300;
int wh = 150;
int barw = 100;
int barh = 15;
float opacity = 0.7; /* window opacity */

char br_cmd[] = "\
BRFILE=/sys/class/backlight/acpi_video0/brightness\n\
BRMAXFILE=/sys/class/backlight/acpi_video0/max_brightness\n\
BRMAX=`cat $BRMAXFILE`\n\
BR=`cat $BRFILE`\n\
echo | awk \"END{print $BR/$BRMAX*100}\"";

char vol_cmd_dec[] = "ossvol.sh -2 >/dev/null; ossvol.sh -v | awk \"END{print \\$0/25*100}\"";
char vol_cmd_inc[] = "ossvol.sh +2 >/dev/null; ossvol.sh -v | awk \"END{print \\$0/25*100}\"";
char vol_cmd_mute[] = "ossvol.sh -t";
char mpd_cmd_toggle[] = "mpc toggle | sed -n -e 's/^\\[\\(.*\\)\\].*/\\1/p' | tr '[=p=]' P";
char mpd_cmd_next[] = "mpc next>/dev/null; s=`mpc current`; [ \"$s\" ] && echo $s || echo Nothing to Play";
char mpd_cmd_prev[] = "mpc prev>/dev/null; s=`mpc current`; [ \"$s\" ] && echo $s || echo Nothing to Play";

/*
 * Note: the shell command stdout is passed to
 * the selected display function.
 * text_with_bar needs a number between 0 and 100.
 * text_with_text needs a string.
 */
struct config
conf[] = {
	/* Key                           Text                 Display func          Shell command */
	{XF86XK_MonBrightnessUp,         "Brightness",        text_with_bar,        br_cmd},
	{XF86XK_MonBrightnessDown,       "Brightness",        text_with_bar,        br_cmd},
	{XF86XK_AudioLowerVolume,        "Volume",            text_with_bar,        vol_cmd_dec},
	{XF86XK_AudioRaiseVolume,        "Volume",            text_with_bar,        vol_cmd_inc},
	{XF86XK_AudioMute,               "Volume",            text_with_text,       "ossvol.sh -t"},
	{XF86XK_AudioPlay,               "MPD",               text_with_text,       mpd_cmd_toggle},
	{XF86XK_AudioNext,               "MPD",               text_with_text,       mpd_cmd_next},
	{XF86XK_AudioPrev,               "MPD",               text_with_text,       mpd_cmd_prev},
};

