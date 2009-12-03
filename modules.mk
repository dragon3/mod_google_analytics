mod_google_analytics.la: mod_google_analytics.slo
	$(SH_LINK) -rpath $(libexecdir) -module -avoid-version  mod_google_analytics.lo
DISTCLEAN_TARGETS = modules.mk
shared =  mod_google_analytics.la
