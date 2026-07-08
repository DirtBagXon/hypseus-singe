![Hypseus Singe](https://raw.githubusercontent.com/DirtBagXon/hypseus-singe/master/screenshots/hypseus-logo.png)

# Hypseus Singe for Windows

## Download

**Download latest version:** [here](https://github.com/DirtBagXon/hypseus-singe/releases)

Latest SDL3 build is:

    hypseus.exe -version
    [version] Hypseus Singe: v3.0.0
    [console] Windows 11
    [console] SDL(CC): 3.4.10
    [console] SDL(LD): 3.4.10

Windows executables linked with `-subsystem,windows` are also available to suppress *Command Prompt* popups.


## Install

Unzip *Hypseus Singe* to its own directory. *e.g.*

    C:\Hypseus Singe

Place zipped *rom* files in the `roms` directory.

Place *framefile* and *video* files in `vldp` or `singe` under the appropriate game directory name.

## Running games

A dedicated Windows Frontend Launcher is now available:

![Borf Launcher](https://raw.githubusercontent.com/DirtBagXon/hypseus-singe/master/screenshots/borfabout.png)

Example `.bat` files are also provided in the repo.

    hypseus.exe lair vldp -scorepanel -framefile vldp/lair/lair.txt

    hypseus.exe singe vldp -framefile singe/timegal/timegal.txt -zlua singe/timegal/timegal.zip

[Arguments](https://github.com/DirtBagXon/hypseus-singe/blob/master/doc/CmdLine.md) can also be organized and exported to `.bat` files via the Borf Launcher.

## Configuration

Configuration in [hypinput.ini](https://github.com/DirtBagXon/hypseus-singe/blob/master/doc/hypinput.ini) within the Hypseus folder.

Use the `hypjsch` application in the `C:\Hypseus Singe` folder to help calculate values.

See also the **Gamepad** configuration option: [hypinput_gamepad.ini](https://github.com/DirtBagXon/hypseus-singe/blob/master/doc/hypinput_gamepad.ini)

**Note:** Hypseus Singe uses **SDL2** keycodes. See information in *hypinput.ini*
