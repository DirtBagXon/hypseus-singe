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
        BLANK="-blank_searches -blank_skips -min_seek_delay 1000"
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
      -overlay)
        OVERLAY="-original_overlay"
        shift
        ;;
      -prototype)
        PROTOTYPE="on"
        shift
        ;;
      -rotate)
        ROTATE="-rotate 90"
        shift
        ;;
      -scale)
        SCALE="-scalefactor 50"
        shift
        ;;
      -scanlines)
        SCANLINES="-scanlines -x 1024 -y 768"
        shift
        ;;
      -scoreboard)
        if [ $FULLSCREEN ] ; then
           FULLSCREEN="-fullscreen_window"
        fi
        SCOREBOARD="-software_scoreboard"
        shift
        ;;
      *)
        POSITIONAL+=("$1")
        shift
        ;;

    esac
done

set -- "${POSITIONAL[@]}"

if [ -z "$1" ] ; then
    echo "Specify a game to try: " | STDERR
    echo
    echo  -e "$0 [-fullscreen] [-blanking] [-nolinear] [-prototype] [-scanlines] [-scoreboard] <gamename>" | STDERR

    for game in ace astron badlands bega blazer cliff cobra cobraab cobram3 dle21 esh galaxy gpworld interstellar lair lair2 mach3 roadblaster sae sdq tq uvt; do
	if ls $HYPSEUS_SHARE/vldp*/$game >/dev/null 2>&1; then
	    installed="$installed $game"
	else
	    uninstalled="$uninstalled $game"
	fi
    done
    if [ "$uninstalled" ]; then
	echo
	echo "Games not found in $HYPSEUS_SHARE/vldp*: " | STDERR
	echo "$uninstalled" | fold -s -w60 | sed 's/^ //; s/^/\t/' | STDERR
	echo
    fi
    if [ -z "$installed" ]; then
	cat <<EOF

Error: Please put the required files in $HYPSEUS_SHARE/vldp_dl/gamename/ or $HYPSEUS_SHARE/vldp/gamename/
EOF
    else
	echo
	echo "Games available: " | STDERR
	echo "$installed" | fold -s -w60 | sed 's/^ //; s/^/\t/' | STDERR
	echo
    fi
    exit 1
fi

case "$1" in
    ace)
	VLDP_DIR="vldp_dl"
	FASTBOOT="-fastboot"
	BANKS="-bank 1 00000001 -bank 0 00000010"
	;;
    astron)
	VLDP_DIR="vldp"
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    badlands)
	VLDP_DIR="vldp"
	BANKS="-bank 1 10000001 -bank 0 00000000"
	;;
    bega)
	VLDP_DIR="vldp"
	;;
    blazer)
	VLDP_DIR="vldp"
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    cliff)
	VLDP_DIR="vldp"
	FASTBOOT="-fastboot"
	BANKS="-bank 1 00000000 -bank 0 00000000 -cheat"
	;;
    cobra)
	VLDP_DIR="vldp"
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    cobraab)
	VLDP_DIR="vldp"
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    cobram3)
	VLDP_DIR="vldp"
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    dle21)
	VLDP_DIR="vldp_dl"

	if [ "$PROTOTYPE" ]; then
		BANKS="-bank 1 10110111 -bank 0 11011001"
	else
		BANKS="-bank 1 00110111 -bank 0 11011001"
	fi
	;;
    esh)
	# Run a fixed ROM so disable CRC
	VLDP_DIR="vldp"
	FASTBOOT="-nocrc"
	;;
    galaxy)
	VLDP_DIR="vldp"
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    gpworld)
	VLDP_DIR="vldp"
	;;
    interstellar)
	VLDP_DIR="vldp"
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    mach3)
	VLDP_DIR="vldp"
	BANKS="-bank 0 01000001"
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    lair)
	VLDP_DIR="vldp_dl"
	FASTBOOT="-fastboot"
	BANKS="-bank 1 00110111 -bank 0 10011001"
	;;
    lair2)
	VLDP_DIR="vldp_dl"
	;;
    roadblaster)
	VLDP_DIR="vldp"
	;;
    sae)
	VLDP_DIR="vldp_dl"
	BANKS="-bank 1 01100111 -bank 0 10011000"
	;;
    sdq)
	VLDP_DIR="vldp"
	BANKS="-bank 1 00000000 -bank 0 00000000"
	;;
    tq)
	VLDP_DIR="vldp_dl"
	BANKS=" -bank 0 00010000"
	;;
    uvt)
	VLDP_DIR="vldp"
	BANKS="-bank 0 01000000"
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    *) echo -e "\nInvalid game selected\n"
       exit 1
esac

if [ ! -f $HYPSEUS_SHARE/$VLDP_DIR/$1/$1.txt ]; then
        echo
        echo "Missing frame file: $HYPSEUS_SHARE/$VLDP_DIR/$1/$1.txt ?" | STDERR
        echo
        exit 1
fi

$HYPSEUS_BIN $1 vldp \
$FASTBOOT \
$FULLSCREEN \
$NEAREST \
$BLANK \
$OVERLAY \
$LOG \
$ROTATE \
$SCANLINES \
$SCALE \
$SCOREBOARD \
$KEYINPUT \
$BANKS \
-framefile $HYPSEUS_SHARE/$VLDP_DIR/$1/$1.txt \
-homedir $HYPSEUS_SHARE \
-datadir $HYPSEUS_SHARE \
-sound_buffer 2048 \
-volume_nonvldp 5 \
-volume_vldp 20

EXIT_CODE=$?

if [ "$EXIT_CODE" -ne "0" ] ; then
       echo "HypseusLoader failed to start, returned: $EXIT_CODE." | STDERR
fi
exit $EXIT_CODE
