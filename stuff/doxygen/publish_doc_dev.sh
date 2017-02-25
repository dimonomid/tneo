#!/bin/bash

# get version string
version_string="$(bash ../scripts/git_ver_echo.sh)"
echo "TNeo version: $version_string"

# generate commit message
new_commit_message="docs updated: $version_string"

# make sure we have docs repo
if ! [ -d ./dfrank.bitbucket.org ]; then
   # clone docs repo
   hg clone ssh://hg@bitbucket.org/dfrank/dfrank.bitbucket.org
fi

# now, docs repo should be cloned in any case, go to it
cd dfrank.bitbucket.org

# make sure we have the latest revision
hg pull -u

# check last commit message
last_commit_message="$(hg log -l1 --template '{desc}')"

# go back
cd ..

# compare it
if [[ "$new_commit_message" == "$last_commit_message" ]]; then
   echo "tneo repository did not change, exiting"
   exit 0
fi


# ---- tneo repo has changed, so, continue ----

# remove all current output
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

# go to dev documentation dir
cd dfrank.bitbucket.org/tneokernel_api/dev

# remove everything from there
rm -r ./*

# copy new data there
cp -r ../../../output/* .

# addremove
hg addremove

# commit
hg ci -m"$new_commit_message"

# push it
# COMMENTED to be more safe
# hg push
echo "--------------------------------------------------------------------------"
echo "DON'T FORGET to push your changes now, if everything is ok:"
echo "    $ cd dfrank.bitbucket.org"
echo "    $ hg push"
echo "--------------------------------------------------------------------------"

# go back
cd ../../..

