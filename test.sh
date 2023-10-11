#!/usr/bin/env sh

if [ -z "$1" ]; then
    echo "Please give an argument... I wish I could tell you more about your options..."
else
    cd dist
    ./"$1".out
fi
