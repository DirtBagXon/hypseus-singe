![Hypseus Singe](https://raw.githubusercontent.com/DirtBagXon/hypseus-singe/master/screenshots/hypseus-minilogo.png)

# RetroPie Install (Hypseus drop-in Daphne replacement)

* **Firstly** check if the 'hypseus' package has been accepted in **RetroPie-Setup** packages.

* Or install the standard 'daphne' plugin via **RetroPie configuration script** and continue below.

* Or follow manual installation instructions from the [forum](https://retropie.org.uk/forum/post/263036) for unsupported platforms.

* Place ROMS as per standard configuration: https://retropie.org.uk/docs/Daphne/

* SSH into the retropie and perform the following to switch `daphne` to `hypseus-singe` manually.


## Install hypseus-singe

    sudo apt-get install libmpeg2-4-dev libsdl2-image-dev libsdl2-ttf-dev

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
    ln -s /opt/retropie/configs/daphne/hypinput.ini /opt/retropie/emulators/daphne/hypinput.ini

You **will** need to remove the ``-nohwaccel`` argument from your ``/opt/retropie/emulators/daphne/daphne.sh``

If you only want ``Daphne`` games, **stop here**.

For extending with ``Singe`` game support, continue below:

## To enable Singe extensions

Link ``singe`` with the emulator path - this requires a custom ``daphne.sh`` which detects singe games:

    cp /opt/retropie/emulators/daphne/daphne.sh /opt/retropie/emulators/daphne/daphne.sh.orig
    cp src/3rdparty/retropie/daphne.sh /opt/retropie/emulators/daphne/daphne.sh

Link a ``singe`` subdirectory to the ``roms`` folder in ``-datadir`` for use as a traversal directory.

    ln -s /home/pi/RetroPie/roms/daphne/roms /opt/retropie/emulators/daphne/singe

### Install Singe games

* Place ``timegal.daphne`` within ``/home/pi/RetroPie/roms/daphne/`` as normal.
* Ensure the main ``.singe`` file matches the game directory name: *i.e.* ``timegal.singe``.

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
    |    |    |-- timegal.singe
    |    |    |-- *.*
    |    |
    |    +-- roms
    |

**Note:** From version **2.6.1** there is no need to link peripheral data in a ``singe`` subdirectory.

This should **complete** the installation.

## Revert to the original Daphne plugin

     mv /opt/retropie/emulators/daphne/daphne.bin.orig /opt/retropie/emulators/daphne/daphne.bin
     rm /opt/retropie/configs/daphne/hypinput.ini /opt/retropie/emulators/daphne/hypinput.ini
     rm -rf /opt/retropie/emulators/daphne/fonts

     mv /opt/retropie/emulators/daphne/daphne.sh.orig /opt/retropie/emulators/daphne/daphne.sh
     rm /opt/retropie/emulators/daphne/singe

Re-add ``-nohwaccel`` to  ``/opt/retropie/emulators/daphne/daphne.sh`` if required.

## Configuration

Follow standard plugin documentation at: https://retropie.org.uk/docs/Daphne/

* However, key and joystick control configuration should be within `hypinput.ini`

## Lightguns

Lightguns may require the ``-manymouse`` argument passed to Singe games to enable absolute mouse inputs.

## Extended argument summary

#### The '-retropath' singe argument explained:

    hypseus singe vldp -retropath -framefile ... -script ...

The Singe specific ``-retropath`` argument, performs an *on-the-fly* rewrite of the data path passed by the game LUA:

    singe/timegal/

...is rewritten to:

    singe/../timegal.daphne/

The ``singe`` subdirectory is now purely traversed so we can symlink from ``-homedir`` to any existing folder in the daphne ``-datadir``.

``roms`` is the obvious choice:

    ls -al /opt/retropie/emulators/daphne/

    drwxr-xr-x 9 pi pi    4096 Jul 12 12:24 .
    -rwxr-xr-x 1 pi pi 2485652 Jul 13 13:00 daphne.bin
    -rwxr-xr-x 1 pi pi     522 Jul 12 12:24 daphne.sh
    drwxr-xr-x 2 pi pi    4096 Jul  9 22:28 fonts
    drwxr-xr-x 2 pi pi    4096 Apr 29 20:23 framefile
    lrwxrwxrwx 1 pi pi      41 Apr 29 20:01 hypinput.ini -> /opt/retropie/configs/daphne/hypinput.ini
    drwxr-xr-x 2 pi pi    4096 Jul 12 12:26 logs
    drwxr-xr-x 3 pi pi    4096 Apr 29 20:00 pics
    drwxr-xr-x 2 pi pi    4096 Jun 28 19:39 ram
    lrwxrwxrwx 1 pi pi      38 Apr 29 20:01 roms -> /home/pi/RetroPie/roms/daphne/roms
    drwxr-xr-x 2 pi pi    4096 Apr 29 20:23 screenshots
    lrwxrwxrwx 1 pi pi      39 Jul 12 00:07 singe -> /home/pi/RetroPie/roms/daphne/roms
    drwxr-xr-x 2 pi pi    4096 Apr 29 20:00 sound

This should allow easier integration within Retro gaming systems.

