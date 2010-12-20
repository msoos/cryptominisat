AM_CXXFLAGS = -Wall $(all_includes) -I$(srcdir) -I$(srcdir)/../MTRand -I$(srcdir)/../mtl $(OPENMP_CXXFLAGS)

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
	OnlyNonLearntBins.cpp SolverConf.cpp \
	MTSolver.cpp BothCache.cpp DataSync.cpp \
	DataSyncServer.cpp
