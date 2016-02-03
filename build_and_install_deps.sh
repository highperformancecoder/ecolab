#!/bin/sh
wget http://download.opensuse.org/repositories/home:hpcoder1/xUbuntu_14.04/Release.key
sudo apt-key add - < Release.key
echo 'deb http://download.opensuse.org/repositories/home:/hpcoder1/xUbuntu_14.04/ /' >/tmp/hpcoders.list
sudo mv /tmp/hpcoders.list /etc/apt/sources.list.d/
sudo apt-get update -qq
sudo apt-get install tcl8.5 tcl8.5-dev tk8.5 tk8.5-dev libreadline5 libboost-graph-dev libigraph0-dev libunuran-dev libboost-graph-dev libreadline5 exuberant-ctags libdb-dev libdb++-dev mpd Xvfb openmpi-bin -y

