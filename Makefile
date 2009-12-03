##
##  Makefile -- Build procedure for sample google_analytics Apache module
##  Autogenerated via ``apxs -n mod_google_analytics -g''.
##

basedir=/usr/share/apache2

# CentOS or RHEL
# basedir=/usr/lib/httpd

top_srcdir=${basedir}
top_builddir=${basedir}

builddir=.

include ${top_builddir}/build/special.mk

#   the used tools
APXS=apxs
APACHECTL=apachectl

#   additional defines, includes and libraries
#DEFS=-Dmy_define=my_value
#INCLUDES=-Imy/include/dir
#LIBS=-Lmy/lib/dir -lmylib

#   the default target
all: local-shared-build

#   install the shared object file into Apache 
install: install-modules-yes

#   cleanup
clean:
	-rm -f mod_google_analytics.o mod_google_analytics.lo mod_google_analytics.slo mod_google_analytics.la 

#   simple test
test: reload
	lynx -mime_header http://localhost/mod_google_analytics

#   install and activate shared object by reloading Apache to
#   force a reload of the shared object file
reload: install restart

#   the general Apache start/restart/stop
#   procedures
start:
	$(APACHECTL) start
restart:
	$(APACHECTL) restart
stop:
	$(APACHECTL) stop

