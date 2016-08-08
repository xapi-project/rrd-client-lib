#! /bin/sh
#
# This script is run inside the Docker xenserver/xenserver-build-env
# container on Travis.
#
set -e

env COMMAND=":" /usr/local/bin/init-container.sh
sudo yum install -y ocaml-rrd-transport-devel
cd /mnt
make
make test
make test-integration
