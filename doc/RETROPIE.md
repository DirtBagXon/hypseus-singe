![Hypseus Singe](https://raw.githubusercontent.com/DirtBagXon/hypseus-singe/master/screenshots/hypseus-logo.png)

# RetroPie Install (Hypseus drop-in Daphne replacement)

## Installation

* `hypseus` is now available in **RetroPie-Setup** (_exp_) packages.

## Configuration

Follow standard plugin documentation at: https://retropie.org.uk/docs/Daphne/

* However, key and joystick control configuration should be within `hypinput.ini`

### Install Singe LUA games

* Place ``timegal.daphne`` within ``/home/pi/RetroPie/roms/daphne/`` as normal.
* Ensure the main ``.singe`` or ``.zip`` ROM file matches the game directory name: *i.e.* ``timegal.zip``.

The file structure is like so:

    roms
    |-- daphne
    |    |
    |    |-- timegal.daphne
    |    |    |
    |    |    |-- timegal.commands  (Optional)
    |    |    |-- timegal.txt
    |    |    |-- timegal.m2v
    |    |    |-- timegal.ogg
    |    |    |-- timegal.zip
    |    |
    |    +-- roms
    |


## Lightguns

Lightguns will require the ``-manymouse`` argument passed to Singe to enable absolute mouse inputs.

See discussion here: [Discussions](https://github.com/DirtBagXon/hypseus-singe/discussions/)

## Compilation

* For compilation the following packages are required:

    sudo apt-get install libmpeg2-4-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libvorbis-dev libogg-dev zlib1g-dev cmake

  *  *  *  *  *

## Extended argument summary

#### The '-retropath' singe argument explained:

    hypseus singe vldp -retropath -framefile ... -script ...

The Singe specific ``-retropath`` argument, performs an *on-the-fly* rewrite of the data path passed by the game LUA:

    singe/timegal/

...is rewritten to:

    roms/../timegal.daphne/

Following ``2.11.2``, if the ``ABSTRACT_SINGE`` option is enabled in ``CMakeLists.txt`` at _build_ time, the rewrite will be:

    roms/../timegal.singe/

This allows _Frontend_ "game type" separation based on folder extensions.

The ``roms`` subdirectory is now purely traversed in the correct ROM location.

    ls -al /opt/retropie/emulators/hypseus/

    drwxr-xr-x 9 root root    4096 Jul 12 12:24 .
    -rwxr-xr-x 1 root root 2485652 Jul 13 13:00 hypseus.bin
    -rwxr-xr-x 1 root root     522 Jul 12 12:24 hypseus.sh
    lrwxrwxrwx 1 root root      36 Apr 29 20:23 bezels -> /opt/retropie/configs/daphne/bezels
    drwxr-xr-x 2 root root    4096 Jul  9 22:28 fonts
    lrwxrwxrwx 1 root root      41 Apr 29 20:01 hypinput.ini -> /opt/retropie/configs/daphne/hypinput.ini
    lrwxrwxrwx 1 root root    4096 Jul 12 12:26 logs -> /opt/retropie/configs/daphne/logs
    drwxr-xr-x 3 root root    4096 Apr 29 20:00 midi
    drwxr-xr-x 3 root root    4096 Apr 29 20:00 pics
    lrwxrwxrwx 1 root root    4096 Jun 28 19:39 ram -> /opt/retropie/configs/daphne/ram
    lrwxrwxrwx 1 root root      38 Apr 29 20:01 roms -> /home/pi/RetroPie/roms/daphne/roms
    lrwxrwxrwx 1 root root    4096 Apr 29 20:23 screenshots -> /opt/retropie/configs/daphne/screenshots
    drwxr-xr-x 2 root root    4096 Apr 29 20:00 sound

This should allow easier integration within Retro gaming systems.

  *  *  *  *  *

## Legacy (manual) install instructions (_for reference_)

* Install the standard 'daphne' plugin via **RetroPie configuration script** and continue below.

* Place ROMS as per standard configuration: https://retropie.org.uk/docs/Daphne/

* SSH into the retropie and perform the following to switch `daphne` to `hypseus-singe` manually.

### Install hypseus-singe

    sudo apt-get install libmpeg2-4-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev

    git clone --single-branch --branch RetroPie https://github.com/DirtBagXon/hypseus-singe.git

    cd hypseus-singe

    mkdir build
    cd build
    cmake ../src
    make

    cd ..

    mv /opt/retropie/emulators/daphne/daphne.bin /opt/retropie/emulators/daphne/daphne.bin.orig
    cp build/hypseus /opt/retropie/emulators/daphne/daphne.bin

    cp -R fonts/ /opt/retropie/emulators/daphne/

    cp doc/hypinput.ini /opt/retropie/configs/daphne/
    ln -sf /opt/retropie/configs/daphne/hypinput.ini /opt/retropie/emulators/daphne/hypinput.ini

You **will** need to remove the ``-nohwaccel`` argument from your ``/opt/retropie/emulators/daphne/daphne.sh``

If you only want ``Daphne`` games, **stop here**.

For extending with ``Singe`` game support, continue below:

### To enable Singe extensions

Link ``singe`` with the emulator path - this requires a custom ``daphne.sh`` which detects singe games:

    cp /opt/retropie/emulators/daphne/daphne.sh /opt/retropie/emulators/daphne/daphne.sh.orig
    cp src/3rdparty/retropie/daphne.sh /opt/retropie/emulators/daphne/daphne.sh

Link a ``singe`` subdirectory to the ``roms`` folder in ``-datadir`` for use as a traversal directory.

    ln -snf /home/pi/RetroPie/roms/daphne/roms /opt/retropie/emulators/daphne/singe

### Revert to the original Daphne plugin

     mv /opt/retropie/emulators/daphne/daphne.bin.orig /opt/retropie/emulators/daphne/daphne.bin
     rm /opt/retropie/configs/daphne/hypinput.ini /opt/retropie/emulators/daphne/hypinput.ini
     rm -rf /opt/retropie/emulators/daphne/fonts

     mv /opt/retropie/emulators/daphne/daphne.sh.orig /opt/retropie/emulators/daphne/daphne.sh
     rm /opt/retropie/emulators/daphne/singe

Re-add ``-nohwaccel`` to  ``/opt/retropie/emulators/daphne/daphne.sh`` if required.

