#!/bin/bash

CURRENT_PATH="$(dirname "$(readlink -f "$0")")"

USER_ID="$(id -u)"
GROUP_ID="$(id -g)"

docker build --rm -t jedi-renderdoc -<<EOF
FROM ubuntu:18.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get -y dist-upgrade && \
    apt-get install -qqy zlib1g-dev curl unzip wget python libc++1 openjdk-8-jdk libproxy1v5 git-lfs python \
                       cmake libqt5x11extras5-dev qt5-default qt5-qmake libqt5svg5-dev automake libpcre3-dev \
                       python3-dev python3-pip python3-tk python3-lxml python3-six libxcb-keysyms1-dev pkg-config \
                       && \
    apt-get clean

RUN groupadd -r $USER -g $GROUP_ID  && adduser --system --GID $GROUP_ID --UID $USER_ID $USER
RUN echo "docker" > /etc/debian_chroot

USER $USER

ENTRYPOINT ["/jedi/docker/docker-entrypoint.sh"]
EOF

docker run -it --rm --network host -v "$CURRENT_PATH/../":/jedi/ jedi-renderdoc

RENDERDOC_LIB_PATH="$(dirname "$(readlink -f "$0")")/../build-python/lib/"
echo "########################################################"
echo "TO USE RENDERDOC, PLEASE RUN THE FOLLOWING COMMANDS:"
echo "$ export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$RENDERDOC_LIB_PATH"
echo "$ export PYTHONPATH=\$PYTHONPATH:$RENDERDOC_LIB_PATH"
echo "########################################################"
