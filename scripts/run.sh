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
      -blanking)
        BLANK="-blank_searches -blank_skips -min_seek_delay 500"
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
      -linear)
        LINEAR="-linear_scale"
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
        SCANLINES="-scanlines"
        shift
        ;;
      -scoreboard)
        if [ $FULLSCREEN ] ; then
           FULLSCREEN="-fullscreen_window"
        fi
        SCOREBOARD="-scorepanel"
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
    echo  -e "$0 [-fullscreen] [-blanking] [-gamepad] [-linear] [-prototype] [-scanlines] [-scoreboard] <gamename>" | STDERR

    for game in ace aceeuro ace91 astron badlands badlandp bega blazer cliff cliffalt cliffalt2 cobra cobraab cobraconv cobram3 dle21 esh eshalt eshalt2 galaxy galaxyp gpworld gtg interstellar lair laireuro lair_ita lair2 mach3 roadblaster sae sdq sdqshort sdqshortalt tq tq_alt tq_swear uvt; do
	if ls $HYPSEUS_SHARE/vldp/$game >/dev/null 2>&1; then
	    installed="$installed $game"
	else
	    uninstalled="$uninstalled $game"
	fi
    done
    if [ "$uninstalled" ]; then
	echo
	echo "Games not found in $HYPSEUS_SHARE/vldp: " | STDERR
	echo "$uninstalled" | fold -s -w60 | sed 's/^ //; s/^/    /' | STDERR
	echo
    fi
    if [ -z "$installed" ]; then
	cat <<EOF

Error: Please put the required files in $HYPSEUS_SHARE/vldp/gamename/
EOF
    else
	echo
	echo "Games available: " | STDERR
	echo "$installed" | fold -s -w60 | sed 's/^ //; s/^/    /' | STDERR
	echo
    fi
    exit 1
fi

case "$1" in
    ace)
	FASTBOOT="-fastboot"
	BANKS="-bank 1 00000001 -bank 0 00000010"
	;;
    aceeuro|ace91)
	;;
    astron)
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    badlands)
	BANKS="-blank_searches -blank_skips -min_seek_delay 600"
	;;
    badlandp)
	BANKS="-spritelite -preset 1"
	;;
    bega)
	;;
    blazer)
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    cliff|cliffalt|cliffalt2)
	FASTBOOT="-fastboot"
	BANKS="-bank 1 00000000 -bank 0 00000000 -cheat"
	;;
    cobra)
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    cobraab)
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    cobraconv)
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    cobram3)
	KEYINPUT="-keymapfile flightkey.ini"
	FASTBOOT="-nocrc"
	;;
    dle21)
	FASTBOOT="-fastboot"

	if [ "$PROTOTYPE" ]; then
		BANKS="-bank 1 10100111 -bank 0 11011000"
	else
		BANKS="-bank 1 00110111 -bank 0 11011000"
	fi
	;;
    esh|eshalt|eshalt2)
	# Run a fixed ROM so disable CRC
	FASTBOOT="-nocrc"
	;;
    galaxy|galaxyp)
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    gpworld)
	;;
    gtg)
	FASTBOOT="-fastboot"
	;;
    interstellar)
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    mach3)
	BANKS="-bank 0 01000001"
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    lair)
	FASTBOOT="-fastboot"
	BANKS="-bank 1 00110111 -bank 0 10011001"
	;;
    laireuro|lair_ita)
	;;
    lair2)
	;;
    roadblaster)
	;;
    sae)
	FASTBOOT="-fastboot"
	BANKS="-bank 1 01100111 -bank 0 10011000"
	;;
    sdq|sdqshort|sdqshortalt)
	BANKS="-bank 1 00000000 -bank 0 00000000"
	;;
    tq|tq_alt|tq_swear)
	BANKS=" -bank 0 00010000"
	;;
    uvt)
	BANKS="-bank 0 01000000"
	KEYINPUT="-keymapfile flightkey.ini"
	;;
    *) echo -e "\nInvalid game selected\n"
       exit 1
esac

if [ ! -f $HYPSEUS_SHARE/vldp/$1/$1.txt ]; then
        echo
        echo "Missing frame file: $HYPSEUS_SHARE/vldp/$1/$1.txt ?" | STDERR
        echo
        exit 1
fi

$HYPSEUS_BIN $1 vldp \
$FASTBOOT \
$FULLSCREEN \
$GAMEPAD \
$LINEAR \
$BLANK \
$OVERLAY \
$LOG \
$ROTATE \
$SCANLINES \
$SCALE \
$SCOREBOARD \
$KEYINPUT \
$BANKS \
-framefile $HYPSEUS_SHARE/vldp/$1/$1.txt \
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
