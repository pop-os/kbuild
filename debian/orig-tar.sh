#!/bin/sh

set -e

SVNROOT=$1
VERSION=$2
REVISION=`svn info $SVNROOT | grep "Last Changed Rev:" | cut -d' ' -f4`

DIR=kbuild-$REVISION
TAR=../kbuild_$VERSION.orig.tar.gz

svn co -r $REVISION $SVNROOT $DIR
tar -c -z  --exclude '*/src/kWorker/tests-gpl2/*' --exclude '*/kBuild/bin*' --exclude '*/out/*' --exclude '*/.svn*' --exclude '*/src/kmk/doc/make.texi' -f $TAR $DIR
rm -rf $DIR

# move to directory 'tarballs'
if [ -r .svn/deb-layout ]; then
  . .svn/deb-layout
  mv $TAR $origDir
  echo "moved $TAR to $origDir"
fi
