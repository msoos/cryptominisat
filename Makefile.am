# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4

#SUBDIRS = Solver
noinst_LTLIBRARIES = libcryptominisat.la

AM_CXXFLAGS = -Wall $(all_includes) -I$(src)/Solver -I$(srcdir)/MTRand -I$(srcdir)/mtl

libcryptominisat_la_SOURCES = Solver/fcopy.cpp Solver/Logger.C Solver/Solver.C Solver/clause.cpp

bin_PROGRAMS = cryptominisat
cryptominisat_LDADD = libcryptominisat.la
cryptominisat_LDFLAGS = -lz -lgmp
cryptominisat_SOURCES = Solver/Main.C
