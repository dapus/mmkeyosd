
char *fontstrbig = "-*-dejavu sans mono-medium-r-*-*-20-*-*-*-*-*-*-*";
char *fontstrsmall = "-*-dejavu sans mono-medium-r-*-*-13-*-*-*-*-*-*-*";
char *fgcolor = "white";
char *bgcolor = "black";
char *errcolor = "red";
int bw = 0;             /* border width    */
int ww = 300;           /* window width    */
int wh = 150;           /* window height   */
int barw = 150;         /* bar width       */
int barh = 15;          /* bar height      */
float opacity = 0.8;    /* window opacity  */

/* this script doesn't actually change
 * brightness (because my hardware does
 * this for me). */
char br_cmd[] = "\
BRFILE=/sys/class/backlight/acpi_video0/brightness\n\
BRMAXFILE=/sys/class/backlight/acpi_video0/max_brightness\n\
BRMAX=`cat $BRMAXFILE`\n\
BR=`cat $BRFILE`\n\
echo | awk \"END{print $BR/$BRMAX*100}\"";

char vol_cmd_dec[] = "amixer set Master 5%- | sed -n -e 's/.*Playback.*\\[\\([0-9]*\\)%\\].*/\\1/p' |  head -n 1";
char vol_cmd_inc[] = "amixer set Master 5%+ | sed -n -e 's/.*Playback.*\\[\\([0-9]*\\)%\\].*/\\1/p' |  head -n 1";
char vol_cmd_mute[] = "amixer set Master toggle | sed -n -e 's/.*Playback.*\\[\\(on\\|off\\)\\].*/\\1/p' | head -n 1 | awk '{ if($0 == \"off\") print \"Muted\"; else print \"Unmuted\"}'";
//char vol_cmd_dec[] = "ossvol.sh -2 | awk \"END{print \\$0/25*100}\"";
//char vol_cmd_inc[] = "ossvol.sh +2 | awk \"END{print \\$0/25*100}\"";
//char vol_cmd_mute[] = "ossvol.sh -t";
char mpd_cmd_toggle[] = "mpc toggle | sed -n -e 's/^\\[\\(.*\\)\\].*/\\1/p' | tr '[=p=]' P | awk '{if($0 == \"Playing\"){printf(\"%s: \", $0); system(\"mpc current\")} else print $0}'";
char mpd_cmd_next[] = "mpc next>/dev/null; s=`mpc current`; [ \"$s\" ] && echo $s || echo Nothing to Play";
char mpd_cmd_prev[] = "mpc prev>/dev/null; s=`mpc current`; [ \"$s\" ] && echo $s || echo Nothing to Play";
char wifi_toggle[] = "[ \"`cat /sys/class/rfkill/rfkill2/state`\" -eq 1 ] && echo On || echo Off";

/*
 * Note: the shell command stdout is passed to
 * the selected display function.
 * text_with_bar needs a number between 0 and 100.
 * text_with_text needs a string.
 */
struct config
conf[] = {
/*   Modifier  Key                           Text                 Display func          Shell command */
    {0,        XF86XK_MonBrightnessUp,         "Brightness",        text_with_bar,        br_cmd},
    {0,        XF86XK_MonBrightnessDown,       "Brightness",        text_with_bar,        br_cmd},
    {0,        XF86XK_AudioLowerVolume,        "Volume",            text_with_bar,        vol_cmd_dec},
    {0,        XF86XK_AudioRaiseVolume,        "Volume",            text_with_bar,        vol_cmd_inc},
    {0,        XF86XK_AudioMute,               "Volume",            text_with_text,       vol_cmd_mute},
    {0,        XF86XK_AudioPlay,               "MPD",               text_with_text,       mpd_cmd_toggle},
    {0,        XF86XK_AudioNext,               "MPD",               text_with_text,       mpd_cmd_next},
    {0,        XF86XK_AudioPrev,               "MPD",               text_with_text,       mpd_cmd_prev},
    {0,        XF86XK_WLAN,                    "Wifi",              text_with_text,       wifi_toggle},
	/* These are just for testing :) */
    {Mod1Mask|ControlMask, XK_m,               "Message",           text_with_text,       "/in/echo \"This is a test :3\""},
    {Mod1Mask|ControlMask, XK_t,               "Message",           text_with_text,       "echo askdj 1>&2"},
};

