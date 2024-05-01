#!/usr/bin/env sh

# Runs a make command for all targets, i.e.:
#   ./make-all

for TARGET in c64 c128 pet plus4; do
    make clean
    make "TARGET=${TARGET}" $@
done
