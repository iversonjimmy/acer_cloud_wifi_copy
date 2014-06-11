#!/bin/bash

# Makefile communicates with submake via shell variables. 
# This script is inserted between two make files to prevent 
# their communication by unsetting the following 
# variables. This way the submake is completely independent.
# We had issues building Android when this separation was not done.
unset MAKEFLAGS MFLAGS MAKELEVEL

#GIT_DIR should point you to the git directory where it contains git's sw_x
cd $GIT_DIR/sw_x/android/2.2/x86
make -j 4 -f build/core/main.mk $@

