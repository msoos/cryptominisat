AM_CXXFLAGS = -Wall $(all_includes) -I$(srcdir)/../ $(OPENMP_CXXFLAGS)

pkginclude_HEADERS = BitArray.h Clause.h CSet.h MatrixFinder.h Solver.h Subsumer.h Watched.h \
	BoundedQueue.h ClauseOffset.h DimacsParser.h GaussianConfig.h OnlyNonLearntBins.h SolverTypes.h \
	time_mem.h XorFinder.h ClauseAllocator.h CompleteDetachReattacher.h DoublePackedRow.h Gaussian.h PackedMatrix.h \
	PropBy.h StateSaver.h UselessBinRemover.h  XorSubsumer.h ClauseCleaner.h constants.h FailedLitSearcher.h \
	PackedRow.h RestartTypeChooser.h StreamBuffer.h VarReplacer.h XSet.h BothCache.h SolverConf.h \
	SCCFinder.h SharedData.h ClauseVivifier.h DataSync.h Alg.h  Heap.h  MersenneTwister.h  Vec.h


noinst_HEADERS = msvc/stdint.h Main.h

lib_LTLIBRARIES = libcryptominisat.la
libcryptominisat_la_LDFLAGS = -release $(VERSION) -no-undefined
libcryptominisat_la_SOURCES = \
	ClauseCleaner.cpp FailedLitSearcher.cpp \
	VarReplacer.cpp \
	MatrixFinder.cpp \
	Solver.cpp XorFinder.cpp \
	PackedRow.cpp Gaussian.cpp StateSaver.cpp \
	RestartTypeChooser.cpp Subsumer.cpp XorSubsumer.cpp \
	SolverMisc.cpp SolverDebug.cpp ClauseAllocator.cpp \
	UselessBinRemover.cpp SCCFinder.cpp ClauseVivifier.cpp \
	CompleteDetachReattacher.cpp DimacsParser.cpp \
	OnlyNonLearntBins.cpp SolverConf.cpp DataSync.cpp \
	BothCache.cpp

bin_PROGRAMS = cryptominisat
cryptominisat_LDADD = libcryptominisat.la
cryptominisat_LDFLAGS = -static -lz $(OPENMP_CXXFLAGS) $(LIBS)
cryptominisat_SOURCES = Main.cpp
