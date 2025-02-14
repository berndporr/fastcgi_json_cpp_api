#!/bin/sh
rm -rf docs
mkdir docs
doxygen
git add docs

