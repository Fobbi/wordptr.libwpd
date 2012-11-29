lib_LTLIBRARIES = libwpd.la
libwpd_la_SOURCES = wp_common.c wp_pool.c wp_string.c wp_configuration.c wp_daemonizer.c 
bin_PROGRAMS = wpd
wpd_SOURCES = wpd.c tests/libwpd_tests.c
wpd_LDADD = libwpd.la
