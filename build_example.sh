#!/usr/bin/env sh

set -xe

CXX="${CXX:-g++}"
CXX_FLAGS=`cat cxxflags.txt`

$CXX $CXX_FLAGS -I. -g -o main $1
