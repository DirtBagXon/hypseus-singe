# RetroPie Install (Hypseus drop-in Daphne replacement)

Install Standard Daphne plugin via **RetroPie configuration script**.

Place ROMS as per standard configuration: https://retropie.org.uk/docs/Daphne/

SSH into the retropie and perform the following to switch `daphne` to `hypseus-singe`.


## Install

    sudo apt-get install libmpeg2-4-dev libsdl2-image-dev libsdl2-ttf-dev

    git clone --single-branch --branch RetroPie https://github.com/DirtBagXon/hypseus-singe.git

    cd hypseus-singe

    mkdir build
    cd build
    cmake ../src
    make -j 2

    cd ..

    mv /opt/retropie/emulators/daphne/daphne.bin /opt/retropie/emulators/daphne/daphne.bin.orig
    cp build/hypseus /opt/retropie/emulators/daphne/daphne.bin

    cp -R fonts/ /opt/retropie/emulators/daphne/

    cp doc/hypinput.ini /opt/retropie/configs/daphne/
    ln -s /opt/retropie/configs/daphne/hypinput.ini /opt/retropie/emulators/daphne/hypinput.ini


## Revert to original Daphne

     mv /opt/retropie/emulators/daphne/daphne.bin.orig /opt/retropie/emulators/daphne/daphne.bin
     rm /opt/retropie/configs/daphne/hypinput.ini /opt/retropie/emulators/daphne/hypinput.ini
     rm -rf /opt/retropie/emulators/daphne/fonts


## Configuration

Follow standard plugin documentation at: https://retropie.org.uk/docs/Daphne/

However, key and joystick control configuration should be within `hypinput.ini`
