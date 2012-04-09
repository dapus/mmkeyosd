
char *fontstr = "-*-dejavu sans mono-medium-r-*-*-20-*-*-*-*-*-*-*";
char *fgcolor = "white";
char *bgcolor = "black";
int bw = 0; /* border width */
int ww = 300;
int wh = 150;
int barw = 100;
int barh = 15;

char br_cmd[] = "\
BRFILE=/sys/class/backlight/acpi_video0/brightness\n\
BRMAXFILE=/sys/class/backlight/acpi_video0/max_brightness\n\
BRMAX=`cat $BRMAXFILE`\n\
BR=`cat $BRFILE`\n\
echo | awk \"END{print $BR/$BRMAX*100}\"";

char vol_cmd_dec[] = "ossvol.sh -2 >/dev/null; ossvol.sh -v | awk \"END{print \\$0/25*100}\"";
char vol_cmd_inc[] = "ossvol.sh +2 >/dev/null; ossvol.sh -v | awk \"END{print \\$0/25*100}\"";
char vol_cmd_mute[] = "ossvol.sh -t";

struct config
conf[] = {
	{XF86XK_MonBrightnessUp,         "Brightness",        text_with_bar,        br_cmd},
	{XF86XK_MonBrightnessDown,       "Brightness",        text_with_bar,        br_cmd},
	{XF86XK_AudioLowerVolume,        "Volume",            text_with_bar,        vol_cmd_dec},
	{XF86XK_AudioRaiseVolume,        "Volume",            text_with_bar,        vol_cmd_inc},
	{XF86XK_AudioMute,               "Volume",            text_with_text,       "ossvol.sh -t"},
};

