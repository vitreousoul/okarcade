#!/usr/bin/env sh

echo "================================"
echo "Desktop builds"
echo "================================"
./build.sh estudioso
./build.sh influence
./build.sh l_system
./build.sh scuba
echo ""


if [ "$1" == "noweb" ]; then
    # skip doing web builds, because they take a long time
    echo "Skipping web builds."
else
    echo "================================"
    echo "Web builds"
    echo "================================"

    source init_emcc.sh

    ./build_web.sh estudioso
    ./build_web.sh influence
    ./build_web.sh l_system
    ./build_web.sh scuba
fi
