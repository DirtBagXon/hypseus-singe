#!/bin/bash

HYPSEUS_BIN=hypseus.bin
HYPSEUS_SHARE=~/.daphne

function STDERR () {
	/bin/cat - 1>&2
}

POSITIONAL=()

while [[ $# -gt 0 ]]; do

    key="$1"

    case $key in
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
      -nolinear)
        NEAREST="-nolinear_scale"
        shift
        ;;
      -nolog)
        LOG="-nolog"
        shift
        ;;
      -oversize)
        OVERSIZE="-oversize_overlay -manymouse"
        shift
        ;;
      -scale)
        SCALE="-scalefactor 50"
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
	echo "$0 [-fullscreen] [-blanking] [-blend] [-nolinear] [-oversize] [-scanlines] [-scale] <gamename>" | STDERR
	echo

        echo "Games available: "
	for game in $(ls $HYPSEUS_SHARE/singe/); do
		if [ $game != "actionmax" ]; then
			installed="$installed $game"
		fi
        done
        echo "$installed" | fold -s -w60 | sed 's/^ //; s/^/    /' | STDERR
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
$BLANK \
$BLEND \
$LOG \
$OVERSIZE \
$SCANLINES \
$SCALE \
$SILENTBOOT \
-sound_buffer 2048 \
-volume_nonvldp 5 \
-volume_vldp 20

EXIT_CODE=$?

if [ "$EXIT_CODE" -ne "0" ] ; then
       echo "HypseusLoader failed to start, returned: $EXIT_CODE." | STDERR
fi
exit $EXIT_CODE
