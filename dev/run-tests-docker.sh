#!/bin/sh
# Simple script to run tests on Docker. See the Dockerfile for more.

set -e

mkdir build
cd build
mkdir images

cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=YES
make
make test
