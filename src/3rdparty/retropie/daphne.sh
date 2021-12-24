#!/bin/bash
dir="$1"
name="${dir##*/}"
name="${name%.*}"

if [[ -f "$dir/$name.commands" ]]; then
    params=$(<"$dir/$name.commands")
fi

if [[ -f "$dir/$name.singe" ]]; then
    "/opt/retropie/emulators/daphne/daphne.bin" singe vldp -retropath -manymouse -framefile "$dir/$name.txt" -script "$dir/$name.singe" -homedir "/opt/retropie/emulators/daphne" -fullscreen $params
else
    "/opt/retropie/emulators/daphne/daphne.bin" "$name" vldp -framefile "$dir/$name.txt" -homedir "/opt/retropie/emulators/daphne" -fullscreen $params
fi
