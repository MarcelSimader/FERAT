#! /usr/bin/bash
# Author: Marcel Simader (marcel.simader@jku.at)
# Date: 30.09.2024
# (c) Marcel Simader 2024, Johannes Kepler Universität Linz

DIR="`dirname "$(realpath "$0")"`"
for file in $(find "$DIR" -name 'test_*' -type f -executable); do
    $file
    echo
done
echo "All done :)"
