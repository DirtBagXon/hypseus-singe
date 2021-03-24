# Hypseus Singe

Hypseus is a fork of [Matt Ownby's][CUS] [Daphne]. A program to play
the original versions of many laserdisc arcade games on a PC or RPi.

This version includes the **Singe** *(1.14)* plug-in.

Features:

* Updated MPEG2 decoder
* Working MPEG2 x86_64 hw accel (SSE2)
* SDL2 support
* [cmake] build tool
* Digital Leisure game overlays
* Singe
* Bugs

## Compile

Minimum software requirements: [gcc], [cmake], [autotools], [zlib], [SDL2],
[libtool], [vorbis] and [ogg].

For RetroPie clone the ``RetroPie`` branch via:

    git clone --single-branch --branch RetroPie https://github.com/DirtBagXon/hypseus-singe.git
    sudo apt-get install libmpeg2-4-dev

Build:

    mkdir build
    cd build
    cmake ../src
    make -j 4

## Install and Run

Ensure you have daphne data in the following `~/.daphne` folders:

    pics, ram, roms, screenshots, sound, vldp and vldp_dl

**From the repo path:**

    cp -R fonts ~/.daphne
    cp doc/hypinput.ini doc/flightkey.ini ~/.daphne
    sudo cp build/hypseus /usr/local/bin/hypseus.bin
    sudo cp ./run.sh /usr/local/bin/hypseus
    sudo cp ./singe.sh /usr/local/bin/singe
    hypseus
    singe


[![Hypseus](https://img.youtube.com/vi/1ZDjMNAxHWw/0.jpg)](https://www.youtube.com/watch?v=1ZDjMNAxHWw)


## Altering Hypseus or Singe ROM locations

Edit **run.sh** and **singe.sh** before copying, to reflect the location of your ROM folders:

    HYPSEUS_SHARE=~/.daphne
    HYPSEUS_SHARE=/home/pi/RetroPie/roms/daphne
    HYPSEUS_SHARE=/home/pi/RetroPie/roms/daphne/singe

## Singe

For Singe, provide arguments to *hypseus* thus:

    hypseus.bin singe vldp -framefile ~/.daphne/singe/timegal/timegal.txt -script ~/.daphne/singe/timegal/timegal.singe -homedir ~/.daphne/ -datadir ~/.daphne/


## Support

This software intended for educational purposes only. Please submit [issues] or
[pull requests] directly to the [project].

**DO NOT submit issues or request support from the official Daphne forums!**

## About

Open development by the original author, [Matt Ownby][CUS], ceased years ago.

This repository was created to build upon the ``SDL2 Hypseus`` project created
by [Jeffrey Clark][JAC]. Many main overlays were still missing in the original
repo and Singe had been completely removed.

The name was changed to _Hypseus_ by Jeffrey so the original authors of
[Daphne] would not be burdened with requests for support.

A big thanks goes out to [Matt Ownby][CUS], [Jeffrey Clark][JAC],
[Manuel Corchete][MAC] and the many other developers who
made their work available for me to build upon. Without them this
project would not be possible.

## License

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

[project]: https://github.com/btolab/hypseus
[issues]: https://github.com/btolab/hypseus/issues
[pull requests]: https://github.com/btolab/hypseus/pulls
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
