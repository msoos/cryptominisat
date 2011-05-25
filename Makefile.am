# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = Solver tests mtl MTRand
EXTRA_DIST = HOWTO_VisualCpp HOWTO_MinGW32 \
	       LICENSE-GPL LICENSE-MIT TODO

profile:
	make clean
	make CXXFLAGS="$(CXXFLAGS) -fprofile-generate" -j8
	make check CXXFLAGS="$(CXXFLAGS) -fprofile-generate"
	make mostlyclean
	make CXXFLAGS="$(CXXFLAGS) -fprofile-use" -j8


all-local: Solver
	cp Solver/cryptominisat .

