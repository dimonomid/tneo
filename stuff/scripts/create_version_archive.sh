#!/bin/bash

# get tag name to generate docs for
target_tag_name="$1"

# compare it
if [[ "$target_tag_name" == "" ]]; then
   echo "usage: $ bash $0 <target tag name>"
   exit 0
fi

# go to root of the repo
cd ../..

# remember path to the repo
repo_path="$(pwd)"

# generate path to temp repo dir
tmp_repo_path="/tmp/tneo_tmp_$target_tag_name"

# remove it if it already exists
if test -d "$tmp_repo_path"; then
   echo "removing $tmp_repo_path.."
   rm -r "$tmp_repo_path"
fi

# clone it from the current one
hg clone $repo_path $tmp_repo_path

# cd to temp dir
cd $tmp_repo_path

# update to the needed revision
hg up $target_tag_name

# copy default cfg as tn_cfg.h
cp src/tn_cfg_default.h src/tn_cfg.h

# make all targets
make -f Makefile-all-arch

# delete obj files
rm -r ./_obj

# delete tn_cfg.h
rm src/tn_cfg.h


# cd to doxygen dir
cd stuff/doxygen

# get version string
version_string="$(bash ../scripts/git_ver_echo.sh)"

# generate archive name (and directory name)
archive_name="tneo-$version_string"

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

# rename refman to tneo.pdf
mv refman.pdf tneo.pdf

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
mkdir -p "$archive_full_name"

# copy data there
rsync -av --exclude=".hg*" --exclude=".vimprj" . "$archive_full_name"

# cd to newly created directory (that will be packed soon)
cd "$archive_full_name"

# remove tn_cfg.h
# COMMENTED because repo is now cloned, and there's no tn_cfg.h
# rm "$archive_full_name/src/tn_cfg.h"

# create doc dirs
mkdir -p doc/{html,pdf}

# copy all html output as is
cp -r stuff/doxygen/output/html/* doc/html

# copy pdf
cp stuff/doxygen/output/latex/tneo.pdf doc/pdf

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
#popd

# update repo to the tip
#hg up tip


