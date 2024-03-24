#!/usr/bin/env sh

echo "================================"
echo "Desktop builds"
echo "================================"
./build.sh estudioso

if [ $? -eq 0 ]; then
    ./build.sh scuba
else
    exit 1
fi

if [ $? -eq 0 ]; then
    ./build.sh l_system
else
    exit 1
fi


echo "================================"
echo "Web builds"
echo "================================"

source init_emcc.sh

./build_web.sh estudioso

if [ $? -eq 0 ]; then
    ./build_web.sh scuba
else
    exit 1
fi

if [ $? -eq 0 ]; then
    ./build_web.sh l_system
else
    exit 1
fi
