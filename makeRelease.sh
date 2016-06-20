echo "old version=`git describe`"
git tag -a -m "" $1
echo '#define VERSION "'$1'"' >include/version.h
echo 'VERSION='$1 >Makefile.version

