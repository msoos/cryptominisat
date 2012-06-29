# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = cmsat man
EXTRA_DIST = HOWTO_VisualCpp HOWTO_MinGW32 \
	       LICENSE-MIT TODO

all-local: cmsat
	cp cmsat/cryptominisat .

