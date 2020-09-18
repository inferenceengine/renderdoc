#!/bin/bash

set -e

cd /jedi/
mkdir -p renderdoc_build
cd renderdoc_build
CC=clang CXX=clang++ CFLAGS="-fPIC -fvisibility=hidden" cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/jedi/dist/ -DVULKAN_LAYER_FOLDER=/jedi/dist/etc/vulkan/implicit_layer.d -DSTATIC_QRENDERDOC=ON -DQRENDERDOC_NO_CXX11_REGEX=ON ../
make -j8
make install

# Copy python modules
mkdir -p /jedi/pymodules
cp -R lib/*.so /jedi/pymodules

# Copy python lib folder, and trim
mkdir -p /jedi/dist/share/renderdoc/pylibs/lib
cd /jedi/dist/share/renderdoc/pylibs/lib
cp -R /usr/lib/python3.6/ .
cd python3.6
# remove cache files
rm -rf $(find -iname __pycache__)
# remove unwanted modules
rm -rf test site-packages ensurepip distutils idlelib config-*

cd /jedi/

# Strip the binaries
strip --strip-unneeded dist/bin/*
strip --strip-unneeded dist/lib/*

# Build android libraries and apks
export ANDROID_HOME=/jedi/android-sdk-linux
export ANDROID_NDK_HOME=/jedi/android-sdk-linux/ndk-bundle/
export ANDROID_SDK=$ANDROID_HOME/
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export PATH=$PATH:$ANDROID_SDK/tools/

if [ ! -d "$ANDROID_HOME" ]
then
  echo "Installing android tools..."
  cd /jedi/
  wget -q https://dl.google.com/android/repository/sdk-tools-linux-4333796.zip -O android-sdk-tools.zip
  unzip -q android-sdk-tools.zip -d ${ANDROID_HOME}
  rm android-sdk-tools.zip
  echo y | $ANDROID_HOME/tools/bin/sdkmanager "platform-tools" "platforms;android-26" "build-tools;29.0.2" ndk-bundle
  cd -
  echo "Installing android tools...done"
fi

# Check that we're set up to build for android
if [ ! -d $ANDROID_SDK/tools ] ; then
	echo "\$ANDROID_SDK is not correctly configured: '$ANDROID_SDK'"
	exit 0;
fi

# Build the arm32 variant
mkdir -p build-android-arm32
pushd build-android-arm32

cmake -DBUILD_ANDROID=1 -DANDROID_ABI=armeabi-v7a -DANDROID_NATIVE_API_LEVEL=23 -DCMAKE_BUILD_TYPE=Release -DSTRIP_ANDROID_LIBRARY=On -DCMAKE_MAKE_PROGRAM=make ..
make -j8

if ! ls bin/*.apk; then
  echo "Android build failed"
  exit 0;
fi

popd # build-android-arm32

mkdir -p build-android-arm64
pushd build-android-arm64

cmake -DBUILD_ANDROID=1 -DANDROID_ABI=arm64-v8a -DANDROID_NATIVE_API_LEVEL=23 -DCMAKE_BUILD_TYPE=Release -DSTRIP_ANDROID_LIBRARY=On -DCMAKE_MAKE_PROGRAM=make ..
make -j8

if ! ls bin/*.apk; then
  echo "Android build failed"
  exit 0;
fi

popd # build-android-arm64
