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
		cryptominisat4/vivifier.cpp

pkgincludesubdir = $(includedir)/cryptominisat4

pkgincludesub_HEADERS = cryptominisat4/solvertypesmini.h \
		cryptominisat4/cryptominisat.h \
		cryptominisat4/solverconf.h

libcryptominisat4_la_LDFLAGS = -release $(RELEASE) -no-undefined

if HAVE_M4RI
libcryptominisat4_la_SOURCES += cryptominisat4/xorfinder.cpp 
libcryptominisat4_la_LIBADD = -lm4ri  $(CRYPTOMINISAT4_M4RI_LDFLAGS)
endif
