AUTOMAKE_OPTIONS = gnu subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS=${DEBUG_FLAGS} ${M4RI_CFLAGS} $(CRYPTOMINISAT4_M4RI_CFLAGS)

lib_LTLIBRARIES = libcryptominisat4.la

libcryptominisat4_la_SOURCES = cryptominisat4/calcdefpolars.cpp \
		cryptominisat4/clauseallocator.cpp \
		cryptominisat4/clausecleaner.cpp \
		cryptominisat4/clause.cpp \
		cryptominisat4/cnf.cpp \
		cryptominisat4/compfinder.cpp \
		cryptominisat4/comphandler.cpp \
		cryptominisat4/completedetachreattacher.cpp \
		cryptominisat4/cryptominisat.cpp \
		cryptominisat4/dimacsparser.cpp \
		cryptominisat4/gatefinder.cpp \
		cryptominisat4/hyperengine.cpp \
		cryptominisat4/implcache.cpp \
		cryptominisat4/prober.cpp \
		cryptominisat4/propengine.cpp \
		cryptominisat4/sccfinder.cpp \
		cryptominisat4/searcher.cpp \
		cryptominisat4/simplifier.cpp \
		cryptominisat4/solutionextender.cpp \
		cryptominisat4/solverconf.cpp \
		cryptominisat4/solver.cpp \
		cryptominisat4/sqlstats.cpp \
		cryptominisat4/stamp.cpp \
		cryptominisat4/strengthener.cpp \
		cryptominisat4/subsumeimplicit.cpp \
		cryptominisat4/subsumestrengthen.cpp \
		cryptominisat4/varreplacer.cpp \
		cryptominisat4/vivifier.cpp \
		cryptominisat4/xorfinder.cpp 

pkgincludesubdir = $(includedir)/cryptominisat4

pkgincludesub_HEADERS = \
		cryptominisat4/alg.h \
		cryptominisat4/avgcalc.h \
		cryptominisat4/bitarray.h \
		cryptominisat4/boundedqueue.h \
		cryptominisat4/calcdefpolars.h \
		cryptominisat4/clabstraction.h \
		cryptominisat4/clauseallocator.h \
		cryptominisat4/clausecleaner.h \
		cryptominisat4/clause.h \
		cryptominisat4/cloffset.h \
		cryptominisat4/cnf.h \
		cryptominisat4/compfinder.h \
		cryptominisat4/comphandler.h \
		cryptominisat4/completedetachreattacher.h \
		cryptominisat4/constants.h \
		cryptominisat4/cryptominisat.h \
		cryptominisat4/cset.h \
		cryptominisat4/dimacsparser.h \
		cryptominisat4/drup.h \
		cryptominisat4/gatefinder.h \
		cryptominisat4/GitSHA1.h \
		cryptominisat4/heap.h \
		cryptominisat4/hyperengine.h \
		cryptominisat4/implcache.h \
		cryptominisat4/main.h \
		cryptominisat4/MersenneTwister.h \
		cryptominisat4/mystack.h \
		cryptominisat4/prober.h \
		cryptominisat4/propbyforgraph.h \
		cryptominisat4/propby.h \
		cryptominisat4/propengine.h \
		cryptominisat4/queue.h \
		cryptominisat4/sccfinder.h \
		cryptominisat4/searcher.h \
		cryptominisat4/simplifier.h \
		cryptominisat4/solutionextender.h \
		cryptominisat4/solverconf.h \
		cryptominisat4/solver.h \
		cryptominisat4/solvertypes.h \
		cryptominisat4/solvertypesmini.h \
		cryptominisat4/sqlstats.h \
		cryptominisat4/stamp.h \
		cryptominisat4/streambuffer.h \
		cryptominisat4/strengthener.h \
		cryptominisat4/subsumeimplicit.h \
		cryptominisat4/subsumestrengthen.h \
		cryptominisat4/time_mem.h \
		cryptominisat4/touchlist.h \
		cryptominisat4/vardata.h \
		cryptominisat4/varreplacer.h \
		cryptominisat4/varupdatehelper.h \
		cryptominisat4/vec.h \
		cryptominisat4/vivifier.h \
		cryptominisat4/watchalgos.h \
		cryptominisat4/watcharray.h \
		cryptominisat4/watched.h \
		cryptominisat4/xorfinderabst.h \
		cryptominisat4/xorfinder.h 

libcryptominisat4_la_LDFLAGS = -release $(RELEASE) -no-undefined
libcryptominisat4_la_LIBADD = $(CRYPTOMINISAT4_M4RI_LDLAGS)
