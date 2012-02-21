#!/bin/bash
cwd=$(pwd)
basedir=$(cd $(dirname $(readlink -e $0));pwd)

if [ ! -f "${basedir}/httpd/bin/apxs" ]; then
    echo "apxs not found ( please run 01_build_httpd.sh )"
    exit 1
fi
if [ ! -f "${basedir}/httpd/modules/mod_google_analytics.so" ]; then
    echo "mod_google_analytics.so not found ( please run 02_build_mod_google_analytics.sh )"
    exit 1
fi

# simple
echo "----------------------------------------------------------------------"
echo ">> Simple Test"
echo "----------------------------------------------------------------------"
rm -f httpd/conf/httpd.conf
cp "${basedir}/conf/httpd-simple.conf" httpd/conf/httpd.conf
perl -pi -e "s|\@\@basedir\@\@|${basedir}|g" httpd/conf/httpd.conf
./httpd/bin/apachectl -f "${basedir}/httpd/conf/httpd.conf"
TEST_ACCOUNT_NUMBER="UA-1234567-8" TEST_MOBILE_ACCOUNT_NUMBER="MO-1234567-8" prove t/
TEST_ACCOUNT_NUMBER="UA-1234567-8" TEST_MOBILE_ACCOUNT_NUMBER="MO-1234567-8" TEST_REFEER="http://github.com" prove t/
./httpd/bin/apachectl stop

sleep 2

# virtualhost
echo "----------------------------------------------------------------------"
echo ">> VirtualHost Setting Test"
echo "----------------------------------------------------------------------"
rm -f httpd/conf/httpd.conf
cp "${basedir}/conf/httpd-vhost.conf" httpd/conf/httpd.conf
perl -pi -e "s|\@\@basedir\@\@|${basedir}|g" httpd/conf/httpd.conf
./httpd/bin/apachectl -f "${basedir}/httpd/conf/httpd.conf"
TEST_ACCOUNT_NUMBER="UA-9876543-2" TEST_MOBILE_ACCOUNT_NUMBER="MO-9876543-2" prove t/
TEST_ACCOUNT_NUMBER="UA-9876543-2" TEST_MOBILE_ACCOUNT_NUMBER="MO-9876543-2" TEST_REFEER="http://google.com" prove t/
./httpd/bin/apachectl stop

sleep 2

# per dir
echo "----------------------------------------------------------------------"
echo ">> Location Setting Test"
echo "----------------------------------------------------------------------"
rm -f httpd/conf/httpd.conf
cp "${basedir}/conf/httpd-perdir.conf" httpd/conf/httpd.conf
perl -pi -e "s|\@\@basedir\@\@|${basedir}|g" httpd/conf/httpd.conf
mkdir -p httpd/htdocs/dir1
cp -a httpd/htdocs/index.html httpd/htdocs/dir1/
./httpd/bin/apachectl -f "${basedir}/httpd/conf/httpd.conf"
TEST_ACCOUNT_NUMBER="UA-3456789-0" TEST_MOBILE_ACCOUNT_NUMBER="MO-3456789-0" TEST_PATH="/dir1/index.html" prove t/
TEST_ACCOUNT_NUMBER="UA-3456789-0" TEST_MOBILE_ACCOUNT_NUMBER="MO-3456789-0" TEST_PATH="/dir1/index.html" TEST_REFERER="/index.html" prove t/
./httpd/bin/apachectl stop

cd ${cwd}
