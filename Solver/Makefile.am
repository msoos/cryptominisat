#noinst_LTLIBRARIES = libcryptominisat.la
lib_LTLIBRARIES = libcryptominisat.la

libcryptominisat_la_LDFLAGS = -release 2.9.1 -no-undefined

AM_CXXFLAGS = -Wall $(all_includes) -I$(srcdir) -I$(srcdir)/../MTRand -I$(srcdir)/../mtl $(OPENMP_CXXFLAGS)

pkgincludesubdir = $(includedir)/cryptominisat/Solver
pkgincludesub_HEADERS = BitArray.h Clause.h CSet.h FindUndef.h MatrixFinder.h PartFinder.h Solver.h Subsumer.h Watched.h \
	BoundedQueue.h ClauseOffset.h DimacsParser.h GaussianConfig.h OnlyNonLearntBins.h PartHandler.h SolverTypes.h \
	time_mem.h XorFinder.h ClauseAllocator.h CompleteDetachReattacher.h DoublePackedRow.h Gaussian.h PackedMatrix.h \
	PropagatedFrom.h StateSaver.h UselessBinRemover.h  XorSubsumer.h ClauseCleaner.h constants.h FailedVarSearcher.h \
	Logger.h PackedRow.h RestartTypeChooser.h StreamBuffer.h VarReplacer.h XSet.h

bin_PROGRAMS = cryptominisat
cryptominisat_LDFLAGS = -lz $(OPENMP_CXXFLAGS)
cryptominisat_SOURCES = Main.cpp \
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
