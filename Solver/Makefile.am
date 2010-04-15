noinst_LTLIBRARIES = libcryptominisat.la

AM_CXXFLAGS = -Wall $(all_includes) -I$(srcdir) -I$(srcdir)/../MTRand -I$(srcdir)/../mtl

libcryptominisat_la_SOURCES = Logger.cpp \
			      Solver.cpp \
			      Gaussian.cpp \
			      PackedRow.cpp \
			      XorFinder.cpp \
			      Conglomerate.cpp \
			      MatrixFinder.cpp \
			      VarReplacer.cpp \
			      FindUndef.cpp


bin_PROGRAMS = cryptominisat
cryptominisat_LDADD = libcryptominisat.la
cryptominisat_LDFLAGS = -lz
cryptominisat_SOURCES = Main.C
