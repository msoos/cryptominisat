# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = Solver tests

profile:
	make clean
	make CXXFLAGS="$(CXXFLAGS) -fprofile-generate" -j8
	make check CXXFLAGS="$(CXXFLAGS) -fprofile-generate"
	make mostlyclean
	make CXXFLAGS="$(CXXFLAGS) -fprofile-use" -j8

pkgincludesubdir = $(includedir)/cryptominisat/mtl
pkgincludesub_HEADERS = mtl/Alg.h  mtl/BasicHeap.h  mtl/BoxedVec.h  mtl/Heap.h  mtl/Map.h  mtl/Queue.h  mtl/Vec.h

pkgincludesubdir = $(includedir)/cryptominisat/MTRand
pkgincludesub_HEADERS = MTRand/MersenneTwister.h



all-local: Solver
	cp Solver/cryptominisat .

