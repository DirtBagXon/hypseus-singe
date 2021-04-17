#!/bin/bash

HYPSEUS_BIN=hypseus.bin
HYPSEUS_SHARE=~/.daphne

function STDERR () {
	/bin/cat - 1>&2
}

if [ "$1" = "-fullscreen" ]; then
    FULLSCREEN="-fullscreen"
    shift
fi

if [ "$1" = "-blend" ]; then
    BLEND="-blend_sprites"
    shift
fi

if [ "$1" = "-nolinear" ]; then
    NEAREST="-fullscreen_scale_nearest"
    shift
fi

if [ -z $1 ] ; then
	echo "Specify a game to try: " | STDERR
	echo
	echo "$0 [-fullscreen] [-blend] [-nolinear] <gamename>" | STDERR
	echo

        echo "Games available: "
	for game in $(ls $HYPSEUS_SHARE/singe/); do
		if [ $game != "actionmax" ]; then
			installed="$installed $game"
		fi
        done
        echo "$installed" | fold -s -w60 | sed 's/^ //; s/^/\t/' | STDERR
	echo
	exit 1
fi

if [ ! -f $HYPSEUS_SHARE/singe/$1/$1.singe ] || [ ! -f $HYPSEUS_SHARE/singe/$1/$1.txt ]; then
        echo
        echo "Missing file: $HYPSEUS_SHARE/singe/$1/$1.singe ?" | STDERR
        echo "              $HYPSEUS_SHARE/singe/$1/$1.txt ?" | STDERR
        echo
        exit 1
fi

$HYPSEUS_BIN singe vldp \
-framefile $HYPSEUS_SHARE/singe/$1/$1.txt \
-script $HYPSEUS_SHARE/singe/$1/$1.singe \
-homedir $HYPSEUS_SHARE \
-datadir $HYPSEUS_SHARE \
$FULLSCREEN \
$NEAREST \
$BLEND \
-sound_buffer 2048 \
-volume_nonvldp 5 \
-volume_vldp 20

EXIT_CODE=$?

if [ "$EXIT_CODE" -ne "0" ] ; then
	if [ "$EXIT_CODE" -eq "127" ]; then
		echo ""
		echo "Hypseus Singe failed to start." | STDERR
		echo "This is probably due to a library problem." | STDERR
		echo "Run hypseus.bin directly to see which libraries are missing." | STDERR
		echo ""
	else
		echo "Loader failed with an unknown exit code : $EXIT_CODE." | STDERR
	fi
	exit $EXIT_CODE
fi
