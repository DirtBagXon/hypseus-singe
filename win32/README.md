# Hypseus Singe for Windows

## Install

Unzip *Hypseus Singe* to its own directory. *e.g.*

    C:\Hypseus Singe

Place zipped *rom* files in the `roms` directory.

Place *framefile* and *video* files in `vldp` or `singe` under the appropriate game name.

## Running games

Example `.bat` files are provided in the repo. Run `hypseus.exe` with Daphne arguments:

    hypseus.exe lair vldp -framefile "c:\Hypseus Singe\vldp\lair\lair.txt" -fullscreen

    hypseus.exe singe vldp -framefile "c:\Hypseus Singe\singe\timegal\timegal.txt" -script "c:\Hypseus Singe\singe\timegal\timegal.singe"

See any additional new arguments on the main page.

## Configuration

Configuration in [hypinput.ini](https://github.com/DirtBagXon/hypseus-singe/blob/master/doc/hypinput.ini) within the directory.

**Note:** Hypseus Singe uses **SDL2** keycodes. See information in *hypinput.ini*
