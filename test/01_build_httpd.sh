#!/bin/bash
basedir=$(cd $(dirname $0);pwd)

rm -rf src httpd
mkdir -p src
cd src
wget http://ftp.riken.jp/net/apache//httpd/httpd-2.2.22.tar.gz
tar zxfv httpd-2.2.22.tar.gz
cd httpd-2.2.22
./configure --prefix="${basedir}/httpd" --enable-so
make
make install
