#! /bin/sh
#
# This script is run inside the Docker xenserver/xenserver-build-env
# container on Travis.
#
set -e

# init-container.sh executes either COMMAND or opens a login shell.
# We just want it to do the initialisation and so we provide a dummy
# command and do everything else explicitly here.
env COMMAND=":" /usr/local/bin/init-container.sh
sudo yum install -y ocaml-rrd-transport-devel
cd /mnt
make
make test
make test-integration
