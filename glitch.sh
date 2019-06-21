#!/bin/sh

if [ -r /usr/local/glitch/Glitch ] && [ -x /usr/local/glitch/Glitch ]
then
    cd /usr/local/glitch && exec ./Glitch -style fusion "$@"
    exit $?
else
    exit 1
fi