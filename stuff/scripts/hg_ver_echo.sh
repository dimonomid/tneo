#!/bin/bash

#  this script echoes version string for the current working copy
#  of the mercurial repository.
#
#  if current working copy is a tagged revision, just a tag is echoed.
#  say, if you have a tag "v1.2", and current working copy is at the
#  revision tagged with it, this script echoes "v1.2".
#
#  if, however, there are some commits after latest tag, it echoes
#  like "BETA v1.2-7", where 7 is a number of commits after latest
#  tag "v1.2".
#
#  if there are no tags at all, "0" is used as a latest tag.

latest_tag="$(hg log -r . --template '{latesttag}')"
latest_tag_dist="$(hg log -r . --template '{latesttagdistance}')"
hash_short="$(hg log -r . --template '{node|short}')"

if [[ "$latest_tag" == "null" ]]; then
   # there are no tags at all, so, just use "0" which looks better than "null"
   latest_tag="0"
fi

if [[ "$latest_tag_dist" == "0" ]]; then
   # this is a stable (at least, tagged) release
   version_string="$latest_tag"
else
   # this is a development release
   version_string="BETA $latest_tag-$latest_tag_dist-$hash_short"
fi

# finally, echo the string
echo -n $version_string

