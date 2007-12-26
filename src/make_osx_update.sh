#!/bin/bash

# check to make sure they've provided a path ..
if [ $# -ne "1" ]
then
	echo "Usage: $0 <path to folder that 'DaphneLoader.app' lives in>"
	exit 1
fi

UPDATEDIR=update_tmp_osx
TMPTAR=daphne_update_osx.tar.gz
APPDIR=$1
SRC_FILES=$APPDIR/"Daphne Loader.app/Contents/MacOS/Daphne.app/Contents/MacOS"
SRC_FRAMEWORKS=$APPDIR/"Daphne Loader.app/Contents/MacOS/Daphne.app/Contents/Frameworks"
DST_FRAMEWORKS=$UPDATEDIR/OSX_Frameworks
UPDATENAME=update_daphne.sh
VERSION=`bash print_version.sh "$SRC_FILES/DaphneManifest.xml"`

# Make sure their second argument points to the right place (we should find the file 'Daphne')
if [ ! -e "$SRC_FILES/Daphne" ]
then
	echo "Are you sure that you specified the folder that 'DaphneLoader.app' _lives_ in?"
	exit 1
fi

# remove all of the old schlop out of it
rm -rf $UPDATEDIR

# create updatedir folder
mkdir -p $UPDATEDIR

# create frameworks folder
mkdir -p $DST_FRAMEWORKS

# copy (and strip) daphne loader executable
cp "$SRC_FILES/Daphne" $UPDATEDIR/.
strip $UPDATEDIR/"Daphne"

# copy files that have changed since base release
cp "$SRC_FILES/daphne-changelog.txt" $UPDATEDIR/.
cp "$SRC_FILES/DaphneManifest.xml" $UPDATEDIR/.

# copy frameworks that have changed since base release
cp -R "$SRC_FRAMEWORKS/GLExtensionWrangler.framework" $DST_FRAMEWORKS

# archive up the update directory
cd $UPDATEDIR
tar cvfz ../$TMPTAR *
cd ..

# add update script to beginning
cat ../wx/frontend/update/updater_pfx.sh $TMPTAR > $UPDATENAME

ZIPNAME=daphne-updater-$VERSION-osx.zip

# create ZIP to be uploaded
zip $ZIPNAME $UPDATENAME

# upload it to server
# (this assumes the server has a matching username for your current username)
scp $ZIPNAME www.daphne-emu.com:/var/www/download/osx/.

# clean-up ...
rm $TMPTAR
rm -r $UPDATEDIR
rm $UPDATENAME
