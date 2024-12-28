#!/bin/bash

set -e

# get version string
version_string="$(bash ../scripts/git_ver_echo.sh)"

# generate commit message
new_commit_message="docs updated: $version_string"

# make sure we have docs repo
if ! [ -d ./dimonomid.github.io ]; then
   # clone docs repo
   git clone git@github.com:dimonomid/dimonomid.github.io
fi

# now, docs repo should be cloned in any case, go to it
cd dimonomid.github.io

# make sure we have the latest revision
git pull origin master

# check last commit message
last_commit_message="$(git log -1 --pretty=%B)"

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
cd dimonomid.github.io/tneokernel_api/dev

# remove everything from there
rm -r ./*

# copy new data there
cp -r ../../../output/* .

# addremove
git add -A

# commit
git commit -m"$new_commit_message"

echo "--------------------------------------------------------------------------"
echo "DON'T FORGET:"
echo "  1. Add the new version link to dimonomid.github.io/tneokernel_api/index.html"
echo "  2. Git push"
echo "--------------------------------------------------------------------------"

# go back
cd ../../..
