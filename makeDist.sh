#!/usr/bin/bash
ECOLAB_VERSION=`git describe`
git archive --format=tar --prefix=ecolab-$ECOLAB_VERSION/ HEAD -o /tmp/ecolab-$ECOLAB_VERSION.tar
# add in submodules
for sub in classdesc classdesc/json_spirit graphcode; do
    pushd $sub
    git archive --format=tar --prefix=ecolab-$ECOLAB_VERSION/$sub/ HEAD -o /tmp/$$.tar
    tar Af /tmp/ecolab-$ECOLAB_VERSION.tar /tmp/$$.tar
    rm /tmp/$$.tar
    popd
done
gzip -f /tmp/ecolab-$ECOLAB_VERSION.tar
