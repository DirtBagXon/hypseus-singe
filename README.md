![Hypseus Singe](https://raw.githubusercontent.com/DirtBagXon/hypseus-singe/master/screenshots/hypseus-logo.png)

# Hypseus Singe

Hypseus is a fork of [Matt Ownby's][CUS] [Daphne].

A program to play laserdisc arcade games on a PC, Mac or Raspberry Pi.

This version includes **Singe** support for Fan Made and [American Laser Games][ALG].

Features:

* SDL2 support
* Updated MPEG2 decoder
* [cmake] build tool
* Singe libretro path integration
* Singe Joystick/Lightgun [mouse] support
* Psuedo Singe 2 support (details below)
* Sinden borders for Singe games
* Alternate overlay choices
* Advanced multi joystick configuration
* Software 'lair/ace' original scoreboard: [preview](screenshots/scoreboard.png)
* Simulated scan lines: [preview](screenshots/scanlines.png)
* Windows and MacOS X Ports

## Windows

**Windows** files and documentation can be found [here](win32/)

## Compile

Minimum software requirements: [gcc], [cmake], [autotools], [zlib], [SDL2],
[libtool], [vorbis] and [ogg].

### MacOS X

Use homebrew to install the following packages:

    brew install cmake
    brew install autoconf
    brew install automake
    brew install pkg-config
    brew install sdl2
    brew install sdl2_ttf
    brew install sdl2_image
    brew install libtool
    brew install libvorbis
    brew install libogg

### Raspberry Pi

For **Raspberry Pi** clone the ``RetroPie`` branch via:

    git clone --single-branch --branch RetroPie https://github.com/DirtBagXon/hypseus-singe.git

Further **RetroPie** and **Raspberry Pi** requirements can be found [here](src/3rdparty/retropie/RETROPIE.md)

### Compilation with CMake

Build:

    mkdir build
    cd build
    cmake ../src
    make -j

## Install and Run

Ensure you have data in the following `daphne` HOME folders:

    pics, ram, roms, sound, singe, (vldp and vldp_dl)

Run `hypseus` with `daphne` [arguments](http://www.daphne-emu.com/mediawiki/index.php/CmdLine) on the command line: Also refer to additional arguments [below](https://github.com/DirtBagXon/hypseus-singe#extended-arguments-and-keys)

    hypseus lair vldp -framefile "vldp_dl\lair\lair.txt" -fullscreen_window -software_scoreboard

    hypseus singe vldp -framefile "singe\timegal\timegal.txt" -script "singe\timegal\timegal.singe"


In **Windows** you are able to create `.bat` files with arguments for specific games.

Example `.bat` files  are provided within the [Windows](win32/) zip file.

**Retro gaming** systems will require adoption within the relevant emulation scripts.  
The singe `-retropath` argument can ease integration into libretro style systems.  
See [RetroPie](src/3rdparty/retropie/RETROPIE.md) as an example.

`bash` scripts are provided for systems that support this shell.

**Install bash scripts:**

    cp -R fonts ~/.daphne
    cp doc/hypinput.ini doc/flightkey.ini ~/.daphne
    sudo cp build/hypseus /usr/local/bin/hypseus.bin
    sudo cp scripts/run.sh /usr/local/bin/hypseus
    sudo cp scripts/singe.sh /usr/local/bin/singe
    hypseus
    singe

## Configuration

Configuration of keycodes and joysticks should be made within [hypinput.ini](https://github.com/DirtBagXon/hypseus-singe/blob/master/doc/hypinput.ini)

Refer to [keylist.txt](https://github.com/DirtBagXon/hypseus-singe/blob/master/doc/keylist.txt) for **SDL2** keycode values.

## Screenshots

*(Click images for YouTube playlist)*

[![Hypseus](https://raw.githubusercontent.com/DirtBagXon/hypseus-singe/master/screenshots/screenshot.png)](https://www.youtube.com/playlist?list=PLRLuhkf2c3OeRoXydn0upKyIBUXNMK13x)

[![singe](https://raw.githubusercontent.com/DirtBagXon/hypseus-singe/master/screenshots/singe2.png)](https://www.youtube.com/playlist?list=PLRLuhkf2c3OeRoXydn0upKyIBUXNMK13x)



## Altering Hypseus or Singe ROM locations in bash scripts

Edit **run.sh** and **singe.sh**, to reflect the location of your ROM folders:

    HYPSEUS_SHARE=~/.daphne
    HYPSEUS_SHARE=/home/pi/RetroPie/roms/daphne

**Note:** The default Hypseus home directory, *created* when run without ``-homedir``:

    ~/.hypseus

## Software Scoreboard

Enable the software external [scoreboard panel](screenshots/scoreboard.png) in lair/ace: `-software_scoreboard`

Works in conjunction with `-fullscreen_window` or normal windowed mode.

## Singe

For Singe, provide the following arguments to *hypseus*:

    hypseus singe vldp -framefile ~/.daphne/singe/timegal/timegal.txt -script ~/.daphne/singe/timegal/timegal.singe -homedir ~/.daphne/ -datadir ~/.daphne/

Check for Singe anomalies and replacement files [here](https://github.com/DirtBagXon/hypseus_singe_data).

## Singe 2

Hypseus Singe now has psuedo support for Singe 2 games.

For current details see: [Hypseus Singe 2 Data](https://github.com/DirtBagXon/hypseus_singe_data)

## Lightguns

Singe supports Sinden and DolphinBar based lightguns with the `-manymouse` argument.

This argument enables *absolute* (``EV_ABS``) mouse input event codes.

**Note:** `-sinden` automatically enables `-manymouse`

## Singe joystick [mouse] support

Singe now automatically interprets **joystick axis** change as mouse movement (*Gun Games*).

Adjust sensitivity via `-js_range <1-20>` in Singe arguments.

Configure **joystick controls** in [hypinput.ini](https://github.com/DirtBagXon/hypseus-singe/blob/master/doc/hypinput.ini)

## Extended arguments and keys

The following additional, and reimplemented, arguments have been added to Hypseus Singe:

    -blank_searches            [ VLDP blanking [adjust: -min_seek_delay]       ]
    -blank_skips               [ VLDP blanking [adjust: -min_seek_delay]       ]
    -force_aspect_ratio        [ Force 4:3 aspect ratio                        ]
    -keymapfile                [ Specify an alternate hypinput.ini file        ]
    -nolinear_scale            [ Disable bilinear scaling [fullscreen]         ]
    -original_overlay          [ Enable daphne style overlays (lair,ace,lair2) ]
    -scalefactor               [ Scale video image [50-100]%                   ]
    -scanlines                 [ Simulate scanlines [adjust: -x -y]            ]
    -software_scoreboard       [ Enable software scoreboard in lair/ace/tq     ]

    -blend_sprites             [ Restore BLENDMODE outline on Singe sprites    ]
    -js_range <1-20>           [ Adjust Singe joystick sensitivity: [def:5]    ]
    -manymouse                 [ Enable absolute mouse input [lightguns]       ]
    -retropath                 [ Singe data path rewrites [libretro]           ]
    -sinden <1-10> <color>     [ Enable software border for lightguns          ]
                               [ Color: (w)hite, (r)ed, (g)reen, (b)lue or (x) ]

    Alt-Enter                  [ Toggle fullscreen                             ]
    Alt-Backspace              [ Toggle scanlines                              ]
    [KEY_BUTTON3]              [ Toggle scoreboard display in lair/ace         ]
    [KEY_COIN1]=|[KEY_START1]  [ Joystick hotkey combination for [KEY_QUIT]    ]


## Support

This software intended for educational purposes only. Please submit [issues] or
[pull requests] directly to the [project].

**DO NOT submit issues or request support from the official Daphne forums!**

## About

Open development by the original author, [Matt Ownby][CUS], ceased years ago.

Singe was created by [Scott Duensing][SD] as a plugin to Daphne to allow the
playing of [American Laser Games][ALG].

This repository was created to build upon the ``Hypseus`` project created
by [Jeffrey Clark][JAC]. Many overlays were still unimplemented in the original
repository. Singe had also been removed.

The name was changed to _Hypseus_ so the original authors of [Daphne] would not
be burdened with requests for support.

A big thanks goes out to [Matt Ownby][CUS], [Scott Duensing][SD], [Jeffrey Clark][JAC],
[Manuel Alfayate][MAC], [David Griffith][DG] and the many other developers
who made their work available for me to build upon. Without them this
project would not be possible.

## License

**Hypseus Singe**, Super Multiple Arcade Laserdisc Emulator  
Copyright (C) 2021  [DirtBagXon][owner]

**Hypseus**, Multiple Arcade Laserdisc Emulator  
Copyright (C) 2016  [Jeffrey Clark][JAC]

**Daphne**, the First Ever Multiple Arcade Laserdisc Emulator  
Copyright (C) 1999-2013  [Matt Ownby][CUS]

[![GNU General Public License version 3](http://www.gnu.org/graphics/gplv3-127x51.png)][GNU General Public License]

    This program is free software: you can redistribute it and/or modify
    it under the terms of the [GNU General Public License] as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    [GNU General Public License] for more details.

## Trademark

The "Hypseus Singe" mark is used to uniquely identify this project as an Arcade
Laserdisc Emulator.  __Any direct or indirect commercial use of the mark
"Hypseus" is strictly prohibited without express permission.__

[owner]: https://github.com/DirtBagXon
[project]: https://github.com/DirtBagXon/hypseus-singe
[issues]: https://github.com/DirtBagXon/hypseus-singe/issues
[pull requests]: https://github.com/DirtBagXon/hypseus-singe/pulls
[Daphne]: http://www.daphne-emu.com
[CUS]: http://www.daphne-emu.com/site3/statement.php
[gcc]: https://gcc.gnu.org/
[zlib]: http://www.zlib.net/
[libmpeg2]: http://libmpeg2.sourceforge.net/
[SDL2]: https://www.libsdl.org/download-2.0.php
[SDL_Image]: https://www.libsdl.org/projects/SDL_image/
[SDL_Mixer]: https://www.libsdl.org/projects/SDL_mixer/
[SDL_ttf]: https://www.libsdl.org/projects/SDL_ttf/
[GLEW]: http://glew.sourceforge.net/
[ogg]: https://en.wikipedia.org/wiki/Ogg
[vorbis]: https://en.wikipedia.org/wiki/Vorbis
[cmake]: https://cmake.org
[autotools]: https://en.wikipedia.org/wiki/GNU_Build_System
[libtool]: https://www.gnu.org/software/libtool/manual/libtool.html
[GNU General Public License]: http://www.gnu.org/licenses/gpl-3.0.en.html
[JAC]: https://github.com/h0tw1r3
[MAC]: https://github.com/vanfanel
[ALG]: https://en.wikipedia.org/wiki/American_Laser_Games
[SD]: https://github.com/sduensin
[DG]: https://github.com/DavidGriffith
