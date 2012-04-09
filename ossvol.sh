#!/bin/sh
#
# ossvol.sh is a simple script to manage oss volume levels and muting.
# I think i found this on arch linux wiki page. But I made some changes
# to it.

## Config
VOLSTORE=~/.volume
CHANNEL="vmix0-outvol"

## Code

usage() {
	echo "Usage: $0 [options]"
	echo
	echo "Options:"
	echo "	+VOL	Increase volume"
	echo "	-VOL	Decrease volume"
	echo "	VOL	Set volume to VOL"
	echo "	-v	Show volume"
	echo "	-t	Toggle mute on and off"
	echo "	-h	Display this help"
	exit 1
}

getvol() {
	echo `ossmix $CHANNEL | awk '{print $10}' | awk -F : '{print $1}'`
}

toggle() {
	if [ -f $VOLSTORE ]; then
		ossmix $CHANNEL `cat $VOLSTORE` >/dev/null
		rm $VOLSTORE
		echo Unmuted
	else
		VOLUME=`getvol`
		ossmix $CHANNEL 0 >/dev/null
		echo $VOLUME > $VOLSTORE
		echo Muted
	fi
}

setvol() {
	ARG="$*"
	NEWVOL=$ARG
	if [ -f $VOLSTORE ]; then
		TMPVOL=`cat $VOLSTORE`
		OP=`echo $ARG | sed -n -e 's/^\([+-]\)[0-9]*/\1/p'`
		if [ "$OP" ]; then
			VOL=`echo $ARG | sed -n -e "s/^$OP\(.*\)/\1/p"`
			NEWVOL="$OP"`expr $TMPVOL $OP $VOL`
		fi
		rm $VOLSTORE
	fi
	ossmix $CHANNEL -- $NEWVOL >/dev/null
	getvol
}

ARG="$*"
A=`echo $ARG | sed -n -e '/^[+-]*[0-9]\+/p'`
if [ "$A" ]; then
	setvol "$1"
	exit
fi

case "$ARG" in
'-t')
	toggle
	;;
'-v')
	getvol
	;;
*)
	usage
	;;
esac

