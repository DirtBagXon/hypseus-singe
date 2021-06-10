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

If you only want ``Daphne`` games, **stop here**.

For extending with ``Singe`` game support, continue below:

## To enable Singe extension

Link ``singe`` within emulator path:

    cp /opt/retropie/emulators/daphne/daphne.sh /opt/retropie/emulators/daphne/daphne.sh.orig
    cp src/3rdparty/retropie/daphne.sh /opt/retropie/emulators/daphne/daphne.sh

    mkdir  /home/pi/RetroPie/roms/daphne/singe
    ln -s /home/pi/RetroPie/roms/daphne/singe /opt/retropie/emulators/daphne/singe

### Enable games within Singe subdirectory

* Place ``timegal.daphne`` within ``/home/pi/RetroPie/roms/daphne/`` as normal.

* Enabling games will depend on the filesystem you have your ``roms`` directory mounted upon:

* See details for ``EXT`` (*Linux*) and ``FAT/NTFS`` (*Windows*) partition types below.

### ``EXT`` (Linux filesystem)

``symlink`` each Singe game path within the ``roms/daphne/singe`` directory as needed:

    cd /home/pi/RetroPie/roms/daphne/singe

    ln -s ../timegal.daphne timegal
    ln -s ../maddog.daphne maddog
    ...


The file structure is like so:


    roms          (EXT linux filesystem)
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
    |    +-- singe
    |         +-- timegal  <<- link (ln -s ../timegal.daphne timegal)


### Windows based ``FAT/NTFS`` filesystems

**You cannot create symlinks on Windows filesystems.**

You therefore need to copy peripheral data to the ``singe`` subdirectory:

    cd /home/pi/RetroPie/roms/daphne/singe
    mkdir timegal
    cd timegal
    tar -cf - --exclude='*.m2v' --exclude='*.ogg' --exclude='*.dat' -C ../../$(basename $(pwd)).daphne/ . | tar -xf -

*This should duplicate less than 1Mb of data*

The file structure is like so:

    roms          (FAT/NTFS filesystem)
    |-- daphne
    |    |
    |    |-- timegal.daphne
    |    |    |
    |    |    |-- timegal.commands  (Optional)
    |    |    |-- timegal.txt
    |    |    |-- timegal.m2v
    |    |    |-- timegal.ogg
    |    |    |-- timegal.singe
    |    |
    |    +-- singe
    |         |-- timegal
                   |
                   |-- *.*  <<- All other files except .m2v & .ogg


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
