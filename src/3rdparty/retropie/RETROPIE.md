![Hypseus Singe](https://raw.githubusercontent.com/DirtBagXon/hypseus-singe/master/screenshots/hypseus-minilogo.png)

# RetroPie Install (Hypseus drop-in Daphne replacement)

* Install Standard Daphne plugin via **RetroPie configuration script**.

* Place ROMS as per standard configuration: https://retropie.org.uk/docs/Daphne/

* SSH into the retropie and perform the following to switch `daphne` to `hypseus-singe`.


## Install hypseus-singe

    sudo apt-get install libmpeg2-4-dev libsdl2-image-dev libsdl2-ttf-dev

    git clone --single-branch --branch RetroPie https://github.com/DirtBagXon/hypseus-singe.git

    cd hypseus-singe

    mkdir build
    cd build
    cmake ../src
    make -j

    cd ..

    mv /opt/retropie/emulators/daphne/daphne.bin /opt/retropie/emulators/daphne/daphne.bin.orig
    cp build/hypseus /opt/retropie/emulators/daphne/daphne.bin

    cp -R fonts/ /opt/retropie/emulators/daphne/

    cp doc/hypinput.ini /opt/retropie/configs/daphne/
    ln -s /opt/retropie/configs/daphne/hypinput.ini /opt/retropie/emulators/daphne/hypinput.ini

You **may** need to remove the ``-nohwaccel`` argument from your ``/opt/retropie/emulators/daphne/daphne.sh``

If you only want ``Daphne`` games, **stop here**.

For extending with ``Singe`` game support, continue below:

## To enable Singe extensions

Link ``singe`` with emulator path - this requires a custom ``daphne.sh``:

    cp /opt/retropie/emulators/daphne/daphne.sh /opt/retropie/emulators/daphne/daphne.sh.orig
    cp src/3rdparty/retropie/daphne.sh /opt/retropie/emulators/daphne/daphne.sh

    mkdir /home/pi/RetroPie/roms/daphne/singe
    touch /home/pi/RetroPie/roms/daphne/singe/.do_not_delete
    ln -s /home/pi/RetroPie/roms/daphne/singe /opt/retropie/emulators/daphne/singe

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
    |    +-- singe (Required but empty)
    |

**Note:** From version **2.6.1** there is no need to link peripheral data in the ``singe`` subdirectory.

## The -retropath singe argument

The Singe specific ``-retropath`` argument, performs an on-the-fly rewrite of the LUA data path:

    singe/timegal/

...is rewritten to:

    singe/../timegal.daphne/

**Note:** The ``singe`` subdirectory is still required but can remain empty.

This should enable easier integration into Retro gaming systems when used in scripts.

## Revert to original Daphne plugin

     mv /opt/retropie/emulators/daphne/daphne.bin.orig /opt/retropie/emulators/daphne/daphne.bin
     rm /opt/retropie/configs/daphne/hypinput.ini /opt/retropie/emulators/daphne/hypinput.ini
     rm -rf /opt/retropie/emulators/daphne/fonts

     mv /opt/retropie/emulators/daphne/daphne.sh.orig /opt/retropie/emulators/daphne/daphne.sh
     rm -rf /home/pi/RetroPie/roms/daphne/singe
     rm /opt/retropie/emulators/daphne/singe


## Configuration

Follow standard plugin documentation at: https://retropie.org.uk/docs/Daphne/

* However, key and joystick control configuration should be within `hypinput.ini`
