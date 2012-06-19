#!/bin/sh

BRFILE=/sys/class/backlight/acpi_video0/brightness
BRMAXFILE=/sys/class/backlight/acpi_video0/max_brightness
BRMAX=`cat $BRMAXFILE`
BR=`cat $BRFILE`
echo | awk "END{print $BR/$BRMAX*100}"

