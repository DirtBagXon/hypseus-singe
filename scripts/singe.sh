#!/bin/bash

HYPSEUS_BIN=hypseus.bin
HYPSEUS_SHARE=~/.hypseus

function STDERR () {
	/bin/cat - 1>&2
}

POSITIONAL=()

while [[ $# -gt 0 ]]; do

    key="$1"

    case $key in
      -8bit)
        EIGHTBIT="-8bit_overlay"
        shift
        ;;
      -blanking)
        BLANK="-blank_searches -blank_skips -min_seek_delay 200"
        shift
        ;;
      -blend)
        BLEND="-blend_sprites"
        shift
        ;;
      -bootsilent)
        SILENTBOOT="-bootsilent"
        shift
        ;;
      -fullscreen)
        FULLSCREEN="-fullscreen"
        shift
        ;;
      -gamepad)
        GAMEPAD="-gamepad"
        shift
        ;;
      -grabmouse)
        GRABMOUSE="-grabmouse"
        shift
        ;;
      -linear)
        LINEAR="-linear_scale"
        shift
        ;;
      -nolog)
        LOG="-nolog"
        shift
        ;;
      -rotate)
        ROTATE="-rotate 90"
        shift
        ;;
      -scale)
        SCALE="-scalefactor 90"
        shift
        ;;
      -scanlines)
        SCANLINES="-scanlines"
        shift
        ;;
      *)
        POSITIONAL+=("$1")
        shift
        ;;

    esac
done

set -- "${POSITIONAL[@]}"

if [ -z $1 ] ; then
	echo "Specify a game to try: " | STDERR
	echo
	echo "$0 [-fullscreen] [-8bit] [-blanking] [-blend] [-linear] [-gamepad] [-grabmouse] [-scanlines] [-scale] <gamename>" | STDERR
	echo

        echo "Games available: "
	for game in $(ls $HYPSEUS_SHARE/singe/); do
		if [ $game != "actionmax" ] && [ $game != "Framework" ]; then
			installed="$installed $game"
		fi
        done
        echo "$installed" | fold -s -w60 | sed 's/^ //; s/^/    /' | STDERR
	echo
	exit 1
fi

ROMSTART="-script"
ROMFILE="$HYPSEUS_SHARE/singe/$1/$1.singe"

if [ ! -f $ROMFILE ]; then
        echo
        echo "Missing: $HYPSEUS_SHARE/singe/$1/$1.singe" | STDERR
        echo "Will attempt to load from Zip..."
        echo
        ROMSTART="-zlua"
        ROMFILE="$HYPSEUS_SHARE/roms/$1.zip"
fi

FRAMEFILE="$HYPSEUS_SHARE/singe/$1/$1.txt"

if [ ! -f $FRAMEFILE ]; then
        echo
        echo "Missing: $HYPSEUS_SHARE/singe/$1/$1.txt" | STDERR
        echo "Will attempt to load from vldp folder..."
        echo
        FRAMEFILE="$HYPSEUS_SHARE/vldp/$1/$1.txt"
fi

$HYPSEUS_BIN singe vldp \
-framefile $FRAMEFILE \
$ROMSTART $ROMFILE \
-homedir $HYPSEUS_SHARE \
-datadir $HYPSEUS_SHARE \
$FULLSCREEN \
$LINEAR \
$BLANK \
$BLEND \
$GAMEPAD \
$GRABMOUSE \
$LOG \
$ROTATE \
$SCANLINES \
$SCALE \
$SILENTBOOT \
$EIGHTBIT \
-sound_buffer 2048 \
-volume_nonvldp 5 \
-volume_vldp 20

EXIT_CODE=$?

if [ "$EXIT_CODE" -ne "0" ] ; then
       echo "HypseusLoader failed to start, returned: $EXIT_CODE." | STDERR
fi
exit $EXIT_CODE
