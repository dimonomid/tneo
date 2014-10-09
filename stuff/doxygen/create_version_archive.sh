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

# make directory that will be packed soon
mkdir $archive_full_name

# copy data there
rsync -av --exclude=".hg*" . $archive_full_name

# cd to newly created directory (that will be packed soon)
cd $archive_full_name

# remove tn_cfg.h
rm $archive_full_name/src/tn_cfg.h

# create doc dirs
mkdir doc{html,pdf}

# copy all html output as is
cp -r stuff/doxygen/output/html/* doc

# copy pdf
cp stuff/doxygen/output/latex/tneokernel.pdf doc/pdf

# delete doxygen output dir
rm -r stuff/doxygen/output

# pack dir


echo "--------------------------------------------------------------------------"
echo "DON'T FORGET to push your changes now, if everything is ok:"
echo "    $ cd dfrank.bitbucket.org"
echo "    $ hg push"
echo "--------------------------------------------------------------------------"

# go back
popd

# update repo to the tip
hg up tip


