#!/bin/bash
cwd=$(pwd)
basedir=$(cd $(dirname $0);pwd)

if [ ! -f "${basedir}/httpd/bin/apxs" ]; then
    echo "apxs not found ( please run ./01_build_httpd.sh )"
    exit 1
fi

pushd ..
perl -pi'.orig' -e "s|basedir=/usr/share/apache2|basedir=${basedir}/httpd|" Makefile
make
make install
make clean
mv Makefile.orig Makefile
popd
