# Hypseus

Hypseus is a fork of [Matt Ownby's][CUS] [Daphne]. A program that "lets one play
the original versions of many laserdisc arcade games on one's PC."

Some features that differ from the original:

* Updated MPEG2 decoder
* Working MPEG2 x86_64 hw accel (SSE2)
* SDL2 support
* [cmake] build tool
* More bugs!

**Overlays have been implemented with SDL2 at last, but they need testing. So please test and report issues.**

## Compile

_Currently there is no documentation for the new build process. Pull requests
gladly accepted._

Minimum software requirements: [gcc], [cmake], [autotools], [zlib], [SDL2],
[vorbis] and [ogg].

    mkdir build
    cd build
    cmake ../src
    make

## Support

This software intended for educational purposes only. Please submit [issues] or
[pull requests] directly to the [project].

**DO NOT submit issues or request support from the official Daphne forums!**

## About

Open development by the original author, [Matt Ownby][CUS], ceased years ago.

My intention for creating this public repository was to clean up the build
process, fix bugs, learn, upgrade, and publish to foster collaborative
development. The name was changed to _Hypseus_ so the original authors of
[Daphne] would not be burdened with requests for support.

A big thanks goes out to [Matt Ownby][CUS] and the many other developers who
made their work available for me to learn from. Without them this project
would not be possible.

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

The "Hypseus" mark is used to uniquely identify this project  as an Arcade
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
[GNU General Public License]: http://www.gnu.org/licenses/gpl-3.0.en.html
[JAC]: https://github.com/h0tw1r3
