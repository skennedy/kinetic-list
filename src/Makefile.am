
bin_PROGRAMS = list-test
list_test_SOURCES = \
        kinetic-frame.c \
        kinetic-list.c \
        main.c

list_test_CFLAGS = @CLUTTER_CFLAGS@ @MX_CFLAGS@ -Wall -Werror -g3 -O0 -ggdb -DPKGDATADIR="\"$(pkgdatadir)\""
list_test_LDFLAGS = @CLUTTER_LIBS@ @MX_LIBS@

CLEANFILES = *~
