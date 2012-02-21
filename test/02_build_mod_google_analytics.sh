#!/bin/bash
cwd=$(pwd)
basedir=$(cd $(dirname $(readlink -e $0));pwd)

if [ ! -f "${basedir}/httpd/bin/apxs" ]; then
    echo "apxs not found ( please run ./01_build_httpd.sh )"
    exit 1
fi

cd ../
perl -pi'.orig' -e "s|basedir=/usr/share/apache2|basedir=${basedir}/httpd|" Makefile
make
make install
mv Makefile.orig Makefile
make clean
cd ${cwd}
