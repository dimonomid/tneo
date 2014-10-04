#!/bin/bash

# get version string
version_string="$(bash ./hg_ver_echo.sh)"

# generate commit message
new_commit_message="docs updated: $version_string"

# make sure we have docs repo
if ! [ -d ./dfrank.bitbucket.org ]; then
   # clone docs repo
   hg clone https://bitbucket.org/dfrank/dfrank.bitbucket.org
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
   echo "tneokernel repository has not changed, exiting"
   exit 0
fi


# ---- tneokernel repo has changed, so, continue ----

# remove all current output
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

# go back
cd ../../..

