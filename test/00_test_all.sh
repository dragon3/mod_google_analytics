#!/bin/bash
basedir=$(cd $(dirname $(readlink -e $0));pwd)
./01_build_httpd.sh
./02_build_mod_google_analytics.sh
./03_do_test.sh
