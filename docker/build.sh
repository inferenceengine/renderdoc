#!/bin/bash

set -e

CURRENT_PATH="$(dirname "$(readlink -f "$0")")"

cd $CURRENT_PATH

USER_ID="$(id -u)"
GROUP_ID="$(id -g)"

docker build --rm -t jedi-renderdoc -f- . <<EOF
FROM ubuntu:xenial

ARG DEBIAN_FRONTEND=noninteractive

RUN groupadd -r $USER -g $GROUP_ID  && adduser --system --GID $GROUP_ID --UID $USER_ID $USER
RUN echo "docker" > /etc/debian_chroot

COPY prepare.sh /
COPY static_tagging.patch /
RUN /prepare.sh

USER $USER

ENTRYPOINT ["/jedi/docker/docker-entrypoint.sh"]
EOF

docker run -i --rm --network host -v "$CURRENT_PATH/../":/jedi/ jedi-renderdoc

cd -
