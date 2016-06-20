echo "old version=`git describe`"
echo '#define VERSION "'$1'"' >include/version.h
echo 'VERSION='$1 >Makefile.version
git commit -a -m "Release $1"
git tag -a -m "" $1

