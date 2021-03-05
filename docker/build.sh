#!/bin/bash
#
# The MIT License (MIT)
#
# Copyright (C) 2021 OPPO LLC
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
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
