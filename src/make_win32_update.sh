#!/bin/bash

UPDATEDIR=update_tmp_win32
UPDATEHELPDIR=update_helper
TMPZIP=update.zip
SRC_FILES=.
UPDATENAME=update_daphne.exe
VERSION=`bash print_version.sh "$SRC_FILES/DaphneManifest.xml"`

# remove all of the old schlop out of it
rm -rf $UPDATEDIR

# create $(UPDATEDIR) folder
mkdir -p $UPDATEDIR

# copy files that have changed since base release
cp "$SRC_FILES/../daphne.exe" $UPDATEDIR/.
cp "$SRC_FILES/../daphne-changelog.txt" $UPDATEDIR/.
cp "$SRC_FILES/../vldp2.dll" $UPDATEDIR/.
cp "$SRC_FILES/../glew32.dll" $UPDATEDIR/.
cp "$SRC_FILES/../inpout32.dll" $UPDATEDIR/.

# archive up the update directory
cd $UPDATEDIR
echo "Creating update.zip"
zip -9 ../$TMPZIP *
cd ..

# archive up the update.exe, supporting DLLs and our new update.zip
cp $TMPZIP $UPDATEHELPDIR/.
ZIPNAME=daphne-updater-$VERSION-win32.zip

# create ZIP to be uploaded
cd $UPDATEHELPDIR
echo "Creating ZIP with updater program"
zip ../$ZIPNAME *
cd ..

# fix permissions to correct ones (zip creates them wrong)
chmod 644 $ZIPNAME

# upload it to server
scp $ZIPNAME matt@www.daphne-emu.com:/var/www/daphne/download/win32-uploads/.

# clean-up ...
rm $TMPZIP
rm -r $UPDATEDIR
rm $ZIPNAME