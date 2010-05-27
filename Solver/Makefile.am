noinst_LTLIBRARIES = libcryptominisat.la

AM_CXXFLAGS = -Wall $(all_includes) -I$(srcdir) -I$(srcdir)/../MTRand -I$(srcdir)/../mtl

libcryptominisat_la_SOURCES = \
	ClauseCleaner.cpp  FailedVarSearcher.cpp Logger.cpp \
	PartFinder.cpp SmallPtr.cpp VarReplacer.cpp \
	Clause.cpp FindUndef.cpp MatrixFinder.cpp \
	PartHandler.cpp Solver.cpp XorFinder.cpp \
	PackedRow.cpp Gaussian.cpp StateSaver.cpp \
	RestartTypeChooser.cpp Subsumer.cpp XorSubsumer.cpp

bin_PROGRAMS = cryptominisat
cryptominisat_LDADD = libcryptominisat.la
cryptominisat_LDFLAGS = -lz
cryptominisat_SOURCES = Main.C
