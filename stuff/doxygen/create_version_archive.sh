#!/bin/bash

# get tag name to generate docs for
target_tag_name="$1"

# compare it
if [[ "$target_tag_name" == "" ]]; then
   echo "usage: $ bash $0 <target tag name>"
   exit 0
fi

# update to this revision
hg up $target_tag_name

# get version string
version_string="$(bash ./hg_ver_echo.sh)"

# generate archive name (and directory name)
archive_name="tneokernel-$version_string"

# get full path to output archive
archive_full_name="/tmp/$archive_name"

echo "target directory: \"$archive_full_name\""

# remove all current doxygen output
rm -r ./output


# make doxygen output (html, latex)
make

# make pdf file
cd output/latex
make

# rename refman to tneokernel.pdf
mv refman.pdf tneokernel.pdf

# go back
cd ../..

# go to root of the repo
pushd ../..

# if target directory exists, remove it
if test -d "$archive_full_name"; then
   echo "target directory exists, removing.."
   rm -r "$archive_full_name"
fi

# make directory that will be packed soon
mkdir "$archive_full_name"

# copy data there
rsync -av --exclude=".hg*" --exclude=".vimprj" . "$archive_full_name"

# cd to newly created directory (that will be packed soon)
cd "$archive_full_name"

# remove tn_cfg.h
rm "$archive_full_name/src/tn_cfg.h"

# create doc dirs
mkdir -p doc/{html,pdf}

# copy all html output as is
cp -r stuff/doxygen/output/html/* doc/html

# copy pdf
cp stuff/doxygen/output/latex/tneokernel.pdf doc/pdf

# delete doxygen output dir
rm -r stuff/doxygen/output

# remove dfrank.bitbucket.org repo
if test -d "stuff/doxygen/dfrank.bitbucket.org"; then
   echo "removing stuff/doxygen/dfrank.bitbucket.org.."
   rm -r "stuff/doxygen/dfrank.bitbucket.org"
fi

# cd one dir back, in order to make paths in the resulting zip archive correct
cd ..


# if target archive exists, remove it
if test -f "$archive_full_name.zip"; then
   echo "target archve exists, removing.."
   rm "$archive_full_name.zip"
fi

# pack dir
zip -r "$archive_full_name.zip" "$archive_name"


echo "--------------------------------------------------------------------------"
echo " resulting archive is saved as $archive_full_name.zip"
echo "--------------------------------------------------------------------------"

# go back
popd

# update repo to the tip
hg up tip


