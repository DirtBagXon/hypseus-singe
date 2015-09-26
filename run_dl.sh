#!/bin/sh

if [ -z $1 ] ; then
	echo "Specify a game to try: ace, lair, lair2, or tq"
	exit
fi

#strace -o strace.txt \
./daphne $1 vldp \
-framefile ~/.daphne/vldp_dl/$1/$1.txt \
-homedir ~/.daphne \
-datadir ~/.daphne \
-blank_searches \
-min_seek_delay 1000 \
-seek_frames_per_ms 20 \
-sound_buffer 2048 \
-x 640 \
-y 480

#-bank 0 11111001 \
#-bank 1 00100111 \
