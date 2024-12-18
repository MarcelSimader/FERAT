#!/bin/sh
if test $# -lt 3
then
    echo "Usage: $0 timeout command arg ..."
    exit 126
fi
ulimit -t $1 -v $2 || { echo "Error: cannot set timeout $1 $2"; exit 126; }
shift 2
eval "$@"
