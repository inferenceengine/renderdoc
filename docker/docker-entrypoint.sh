#!/bin/bash

set -e

export ANDROID_HOME=/jedi/android-sdk-linux
export ANDROID_NDK_HOME=/jedi/android-sdk-linux/ndk-bundle/
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64

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

cd /jedi/

mkdir -p build-python
cd build-python
cmake -DENABLE_PYRENDERDOC=1 -DENABLE_GL=Off -DENABLE_XLIB=Off -DENABLE_XCB=Off -DENABLE_VULKAN=Off ..
make -j8

cd -
mkdir -p build-android-arm32
cd build-android-arm32
cmake -DBUILD_ANDROID=1 -DANDROID_ABI=armeabi-v7a -DANDROID_NATIVE_API_LEVEL=23 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=make ..
make -j8
cd -

mkdir -p build-android-arm64
cd build-android-arm64
cmake -DBUILD_ANDROID=1 -DANDROID_ABI=arm64-v8a -DANDROID_NATIVE_API_LEVEL=23 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=make ..
make -j8
cd -
