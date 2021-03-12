#!/bin/bash

SCRIPT_DIR=`dirname "$0"`
if realpath / >/dev/null; then SCRIPT_DIR=$(realpath "$SCRIPT_DIR"); fi
DAPHNE_BIN=hypseus
DAPHNE_SHARE=~/.daphne

function STDERR () {
	/bin/cat - 1>&2
}

echo "Daphne Launcher : Script dir is $SCRIPT_DIR"
cd "$SCRIPT_DIR"

if [ "$1" = "-fullscreen" ]; then
    FULLSCREEN="-fullscreen"
    shift
fi

if [[ $@ == *"-prototype"* ]]; then
    PROTOTYPE="on"
fi

if [ -z "$1" ] ; then
    echo "Specify a game to try: " | STDERR
    echo
    echo  -e "\t$0 [-fullscreen] <gamename> [-prototype]" | STDERR

    for game in ace astron badlands bega blazer cliff cobraab dle21 esh galaxy gpworld interstellar lair lair2 mach3 roadblaster sae sdq tq uvt; do
	if ls ~/.daphne/vldp*/$game >/dev/null 2>&1; then
	    installed="$installed $game"
	else
	    uninstalled="$uninstalled $game"
	fi
    done
    if [ "$uninstalled" ]; then
	echo
	echo "Games not found in ~/.daphne/vldp*: " | STDERR
	echo "$uninstalled" | fold -s -w60 | sed 's/^ //; s/^/\t/' | STDERR
    fi
    if [ -z "$installed" ]; then
	cat <<EOF

Error: Please put the required files in ~/.daphne/vldp_dl/gamename/ or ~/.daphne/vldp/gamename/
EOF
    else
	echo
	echo "Games available: " | STDERR
	echo "$installed" | fold -s -w60 | sed 's/^ //; s/^/\t/' | STDERR
    fi
    exit 1
fi

case "$1" in
    ace)
	VLDP_DIR="vldp_dl"
	FASTBOOT="-fastboot"
	BANKS="-bank 1 00100001 -bank 0 00000010"
	SCREENSIZE="-x 720 -y 480"
	;;
    astron)
	VLDP_DIR="vldp"
	KEYINPUT="-keymapfile flightkey.ini"
	SCREENSIZE="-x 512 -y 480"
	;;
    badlands)
	VLDP_DIR="vldp"
	BANKS="-bank 1 10000001 -bank 0 00000000"
	SCREENSIZE="-x 640 -y 480"
	;;
    bega)
	VLDP_DIR="vldp"
	SCREENSIZE="-x 512 -y 480"
	;;
    blazer)
	VLDP_DIR="vldp"
	KEYINPUT="-keymapfile flightkey.ini"
	SCREENSIZE="-x 512 -y 480"
	;;
    cliff)
	VLDP_DIR="vldp"
	FASTBOOT="-fastboot"
	BANKS="-bank 1 01000000 -bank 0 00000000 -cheat"
	SCREENSIZE="-x 640 -y 480"
	;;
    cobraab)
	VLDP_DIR="vldp"
	KEYINPUT="-keymapfile flightkey.ini"
	BANKS="-cheat"
	SCREENSIZE="-x 512 -y 480"
	;;
    dle21)
	VLDP_DIR="vldp_dl"
	SCREENSIZE="-x 640 -y 480"

	if [ "$PROTOTYPE" ]; then
		BANKS="-bank 1 10110011 -bank 0 11011000"
	else
		BANKS="-bank 1 00110011 -bank 0 11011000"
	fi
	;;
    esh)
	# Run a fixed ROM so disable CRC
	VLDP_DIR="vldp"
	FASTBOOT="-nocrc"
	BANKS="-cheat"
	SCREENSIZE="-x 512 -y 480"
	;;
    galaxy)
	VLDP_DIR="vldp"
	KEYINPUT="-keymapfile flightkey.ini"
	SCREENSIZE="-x 512 -y 480"
	;;
    gpworld)
	VLDP_DIR="vldp"
	SCREENSIZE="-x 720 -y 480"
	;;
    interstellar)
	VLDP_DIR="vldp"
	KEYINPUT="-keymapfile flightkey.ini"
	SCREENSIZE="-x 512 -y 480"
	;;
    mach3)
	VLDP_DIR="vldp"
	BANKS="-bank 0 01000001"
	KEYINPUT="-keymapfile flightkey.ini"
	SCREENSIZE="-x 512 -y 480"
	;;
    lair)
	VLDP_DIR="vldp_dl"
	FASTBOOT="-fastboot"
	BANKS="-bank 1 00110011 -bank 0 10011000"
	SCREENSIZE="-x 640 -y 480"
	;;
    lair2)
	VLDP_DIR="vldp_dl"
	BANKS="-cheat"
	SCREENSIZE="-x 640 -y 480"
	;;
    roadblaster)
	VLDP_DIR="vldp"
	BANKS="-cheat"
	SCREENSIZE="-x 512 -y 480"
	;;
    sae)
	VLDP_DIR="vldp_dl"
	BANKS="-bank 1 01100011 -bank 0 10011000"
	SCREENSIZE="-x 720 -y 480"
	;;
    sdq)
	VLDP_DIR="vldp"
	#BANKS="-bank 1 00000000 -bank 0 00000001"
	BANKS="-bank 1 00000000 -bank 0 00000000"
	SCREENSIZE="-x 512 -y 480"
	;;
    tq)
	VLDP_DIR="vldp_dl"
	BANKS=" -bank 0 00010000"
	SCREENSIZE="-x 640 -y 480"
	;;
    uvt)
	VLDP_DIR="vldp"
	BANKS="-bank 0 01000001"
	KEYINPUT="-keymapfile flightkey.ini"
	SCREENSIZE="-x 512 -y 480"
	;;
    *) echo -e "\nInvalid game selected\n"
       exit 1
esac

if [ ! -f $DAPHNE_SHARE/$VLDP_DIR/$1/$1.txt ]; then
        echo
        echo "Missing frame file: $DAPHNE_SHARE/$VLDP_DIR/$1/$1.txt ?" | STDERR
        echo
        exit 1
fi

#strace -o strace.txt \
./$DAPHNE_BIN $1 vldp \
$FASTBOOT \
$FULLSCREEN \
$KEYINPUT \
$BANKS \
-framefile $DAPHNE_SHARE/$VLDP_DIR/$1/$1.txt \
-homedir $DAPHNE_SHARE \
-datadir $DAPHNE_SHARE \
-useoverlaysb 3 \
-sound_buffer 2048 \
-nojoystick \
-volume_nonvldp 5 \
-volume_vldp 20 \
$SCREENSIZE

#-blank_searches \
#-min_seek_delay 1000 \
#-seek_frames_per_ms 20 \

EXIT_CODE=$?

if [ "$EXIT_CODE" -ne "0" ] ; then
	if [ "$EXIT_CODE" -eq "127" ]; then
		echo ""
		echo "Hypseus failed to start." | STDERR
		echo "This is probably due to a library problem." | STDERR
		echo "Run ./hypseus directly to see which libraries are missing." | STDERR
		echo ""
	else
		echo "HypseusLoader failed with an unknown exit code : $EXIT_CODE." | STDERR
	fi
	exit $EXIT_CODE
fi
