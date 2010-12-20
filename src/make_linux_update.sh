#!/bin/bash

UPDATEDIR=update_tmp_linux
DST_LIBDIR=$UPDATEDIR/lib
TMPTAR=daphne_update_linux.tar.gz
SRC_FILES=.
UPDATENAME=update_daphne.sh
VERSION=`bash print_version.sh "$SRC_FILES/DaphneManifest.xml"`

# remove all of the old schlop out of it
rm -rf $UPDATEDIR

# create $(UPDATEDIR) folder
mkdir -p $UPDATEDIR

# create lib dir
mkdir -p $DST_LIBDIR

# copy (and strip) daphne loader executable
cp "$SRC_FILES/../daphne.bin" $UPDATEDIR/.
strip $UPDATEDIR/"daphne.bin"

# copy files that have changed since base release
cp "$SRC_FILES/../daphne" $UPDATEDIR/.
cp "$SRC_FILES/../daphne-changelog.txt" $UPDATEDIR/.
cp "$SRC_FILES/../libvldp2.so" $UPDATEDIR/.
cp "$SRC_FILES/DaphneManifest.xml" $UPDATEDIR/.

# copy library files that may not be common across all distros
# (discovered that Debian Testing's ogg libs have changed, 6Oct2007)
cp /usr/lib/libogg.so.0 $DST_LIBDIR/.
cp /usr/lib/libvorbis.so.0 $DST_LIBDIR/.
cp /usr/lib/libvorbisfile.so.3 $DST_LIBDIR/.

# archive up the update directory
cd $UPDATEDIR
tar cvfz ../$TMPTAR *
cd ..

# add update script to beginning
cat ../wx/frontend/update/updater_pfx.sh $TMPTAR > $UPDATENAME

ZIPNAME=daphne-updater-$VERSION-linux.zip

# create ZIP to be uploaded
zip $ZIPNAME $UPDATENAME

# upload it to server
scp $ZIPNAME matt@www.daphne-emu.com:/var/www/daphne/download/linux/.

# clean-up ...
rm $TMPTAR
rm -r $UPDATEDIR
rm $UPDATENAME
