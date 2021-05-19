# Hypseus Singe for Windows

## Download

To avoid repository pollution with outdated zip files, distribution has been moved to **MEGA**:

**Download latest version:** [here](https://mega.nz/file/59MFHaTa#0k6OLQblvy3uVR6yGsZOEsaz0MGRdFOIwEibRcwebfE)

Latest build is:

    hypseus.exe -v
    [version] Hypseus Singe: v2.5.2
    [console] SDL(CC): 2.0.14
    [console] SDL(LD): 2.0.14
    [console] Build: May 19 2021 16:56:10


## Install

Unzip *Hypseus Singe* to its own directory. *e.g.*

    C:\Hypseus Singe

Place zipped *rom* files in the `roms` directory.

Place *framefile* and *video* files in `vldp` or `singe` under the appropriate game directory name.

## Running games

Example `.bat` files are provided in the repo. Run `hypseus.exe` with *Daphne* [arguments](http://www.daphne-emu.com/mediawiki/index.php/CmdLine):

    hypseus.exe lair vldp -software_scoreboard -framefile "vldp\lair\lair.txt" -fullscreen_window

    hypseus.exe singe vldp -blend_sprites -framefile "singe\timegal\timegal.txt" -script "singe\timegal\timegal.singe"

Refer to additional arguments on the [main page](https://github.com/DirtBagXon/hypseus-singe#extended-arguments-and-keys).

## Configuration

Configuration in [hypinput.ini](https://github.com/DirtBagXon/hypseus-singe/blob/master/doc/hypinput.ini) within the directory.

**Note:** Hypseus Singe uses **SDL2** keycodes. See information in *hypinput.ini*
