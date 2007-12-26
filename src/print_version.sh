#!/bin/bash

# PURPOSE: print the version of the daphne executable (using DaphneManifest.xml) in order
#  to automate building updates.

# sed command explained (so I can remember later):
# '	start quote so our stuff doesn't get parsed wrong
# s	substitution command, common
# :	delimiter, can also use /, _, or |.  I chose : so I wouldn't have to escape stuff.
# [^0-9]	match any charcter that is not 0-9
# :	delimiter
#	replace all matching charcters with emptiness (ie erase them)
# :	delimiter
# g	Apply this to all matching character, not just the first match (g = global)
# '	end quote

# path to DaphneManifest.xml file
MANIFEST=DaphneManifest.xml

# if user passed in a second argument, that becomes the new path
if [ $# -eq "1" ]
then
MANIFEST=$1
fi

MAJOR=`cat "$MANIFEST" | grep major | sed 's:[^0-9]::g'`
MINOR=`cat "$MANIFEST" | grep minor | sed 's:[^0-9]::g'`
BUILD=`cat "$MANIFEST" | grep build | sed 's:[^0-9]::g'`
echo -n $MAJOR.$MINOR.$BUILD
