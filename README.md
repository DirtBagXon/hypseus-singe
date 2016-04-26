# Daphne Fork: the First Ever Multiple Arcade Laserdisc Emulator

[Daphne] is a program that "lets one play the original versions of many
laserdisc arcade games on one's PC".

## Compile

In addition to a working [gcc] toolchain and [cmake], you will need
(at minimum): [zlib], [GLEW], [libmpeg2], [SDL], [vorbis] and [ogg].

    mkdir build
    cd build
    cmake ../src
    make

## Support

This is software is for educational purposes only. You may submit
[issues] or [pull requests] to the [Github page].

**DO NOT submit issues or request support from the official Daphne forums!**

## About

Open development by the original author, [Matt Ownby][CUS], was halted years ago.
My intention for creating this public repository was to clean up the build
process, and publish to Github to foster collaborative development.

[Github page]: https://github.com/zaplabs/daphne
[issues]: https://github.com/zaplabs/daphne/issues
[pull requests]: https://github.com/zaplabs/daphne/pulls
[Daphne]: http://www.daphne-emu.com
[CUS]: http://www.daphne-emu.com/site3/statement.php
[gcc]: https://gcc.gnu.org/
[zlib]: http://www.zlib.net/
[libmpeg2]: http://libmpeg2.sourceforge.net/
[SDL]: https://www.libsdl.org/download-1.2.php
[SDL_Image]: https://www.libsdl.org/projects/SDL_image/release-1.2.html
[SDL_Mixer]: https://www.libsdl.org/projects/SDL_mixer/release-1.2.html
[SDL_ttf]: https://www.libsdl.org/projects/SDL_ttf/release-1.2.html
[GLEW]: http://glew.sourceforge.net/
[ogg]: https://en.wikipedia.org/wiki/Ogg
[vorbis]: https://en.wikipedia.org/wiki/Vorbis
[cmake]: https://cmake.org
