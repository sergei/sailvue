# Build
#BUILD_DIR=cmake-build-debug
BUILD_DIR=cmake-build-release

echo "Building ..."
/Applications/CLion.app/Contents/bin/cmake/mac/x64/bin/cmake --build ./${BUILD_DIR} --target sailvue -j 6

# Make .DMG
echo "Creating .DMG ..."
rm ${BUILD_DIR}/sailvue.dmg
~/Qt/6.5.2/macos/bin/macdeployqt ${BUILD_DIR}/sailvue.app -qmldir=gui -dmg

echo "Created " ${BUILD_DIR}/sailvue.dmg
open ${BUILD_DIR}/sailvue.dmg
