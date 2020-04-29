#!/bin/bash
curdir=$pwd

# Download apron
svn co svn://scm.gforge.inria.fr/svnroot/apron/apron/trunk apron
cd apron

# Prepare the makefile and installation dir
mkdir installed
cp Makefile.config.model Makefile.config
patch < ../Makefile.config.patch

# Build
make -C . APRON_PREFIX=./installed MLGMPIDL_PREFIX=./installed JAVA_PREFIX=./installed
make -C . install

cd $curdir
