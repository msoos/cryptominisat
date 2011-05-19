#noinst_LTLIBRARIES = libcryptominisat.la
lib_LTLIBRARIES = libcryptominisat-2.9.1.la

libcryptominisat_2_9_1_la_LDFLAGS = -release 2.9.1 -no-undefined

AM_CXXFLAGS = -Wall $(all_includes) -I$(srcdir) -I$(srcdir)/../MTRand -I$(srcdir)/../mtl $(OPENMP_CXXFLAGS)

pkgincludesubdir = $(includedir)/cryptominisat/Solver
pkgincludesub_HEADERS = BitArray.h Clause.h CSet.h MatrixFinder.h PartFinder.h Solver.h Subsumer.h Watched.h \
	BoundedQueue.h ClauseOffset.h DimacsParser.h GaussianConfig.h OnlyNonLearntBins.h PartHandler.h SolverTypes.h \
	time_mem.h XorFinder.h ClauseAllocator.h CompleteDetachReattacher.h DoublePackedRow.h Gaussian.h PackedMatrix.h \
	PropBy.h StateSaver.h UselessBinRemover.h  XorSubsumer.h ClauseCleaner.h constants.h FailedLitSearcher.h \
	Logger.h PackedRow.h RestartTypeChooser.h StreamBuffer.h VarReplacer.h XSet.h BothCache.h

libcryptominisat_2_9_1_la_SOURCES = \
	ClauseCleaner.cpp FailedLitSearcher.cpp \
	PartFinder.cpp VarReplacer.cpp \
	MatrixFinder.cpp \
	PartHandler.cpp Solver.cpp XorFinder.cpp \
	PackedRow.cpp Gaussian.cpp StateSaver.cpp \
	RestartTypeChooser.cpp Subsumer.cpp XorSubsumer.cpp \
	SolverMisc.cpp SolverDebug.cpp ClauseAllocator.cpp \
	UselessBinRemover.cpp SCCFinder.cpp ClauseVivifier.cpp \
	CompleteDetachReattacher.cpp DimacsParser.cpp \
	OnlyNonLearntBins.cpp SolverConf.cpp DataSync.cpp \
	BothCache.cpp

bin_PROGRAMS = cryptominisat
cryptominisat_LDADD = libcryptominisat-2.9.1.la
cryptominisat_LDFLAGS = -lz $(OPENMP_CXXFLAGS)
cryptominisat_SOURCES = Main.cpp
