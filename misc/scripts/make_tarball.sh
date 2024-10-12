#!/bin/sh

if [ ! -e "version.py" ]; then
  echo "This script should be ran from the root folder of the Tekisasu Engine repository."
  exit 1
fi

while getopts "h?sv:g:" opt; do
  case "$opt" in
  h|\?)
    echo "Usage: $0 [OPTIONS...]"
    echo
    echo "  -s script friendly file name (tekisasuengine.tar.gz)"
    echo "  -v tekisasuengine version for file name (e.g. 4.0-stable)"
    echo "  -g git treeish to archive (e.g. master)"
    echo
    exit 1
    ;;
  s)
    script_friendly_name=1
    ;;
  v)
    tekisasuengine_version=$OPTARG
    ;;
  g)
    git_treeish=$OPTARG
    ;;
  esac
done

if [ ! -z "$git_treeish" ]; then
  HEAD=$(git rev-parse $git_treeish)
else
  HEAD=$(git rev-parse HEAD)
fi

if [ ! -z "$script_friendly_name" ]; then
  NAME=tekisasuengine
else
  if [ ! -z "$tekisasuengine_version" ]; then
    NAME=tekisasuengine-$tekisasuengine_version
  else
    NAME=tekisasuengine-$HEAD
  fi
fi

CURDIR=$(pwd)
TMPDIR=$(mktemp -d -t tekisasuengine-XXXXXX)

echo "Generating tarball for revision $HEAD with folder name '$NAME'."
echo
echo "The tarball will be written to the parent folder:"
echo "    $(dirname $CURDIR)/$NAME.tar.gz"

git archive $HEAD --prefix=$NAME/ -o $TMPDIR/$NAME.tar

# Adding custom .git/HEAD to tarball so that we can generate VERSION_HASH.
cd $TMPDIR
mkdir -p $NAME/.git
echo $HEAD > $NAME/.git/HEAD
tar -uf $NAME.tar $NAME

cd $CURDIR
gzip -c $TMPDIR/$NAME.tar > ../$NAME.tar.gz

rm -rf $TMPDIR
