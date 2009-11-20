AM_CXXFLAGS = -Wall $(all_includes) -I$(srcdir)/SatElite -I$(srcdir)/ForMani/ADTs -I$(srcdir)/ForMani/Global

bin_PROGRAMS = satelite
satelite_LDFLAGS = -lz
satelite_SOURCES = SatELite/Solver.C \
                   SatELite/Solver_subsume.C \
                   SatELite/TmpFiles.C \
                   SatELite/Main.C \
                   SatELite/Main_debug.C \
                   SatELite/Profile.C \
                   ForMani/ADTs/File.C \
                   ForMani/Global/Global.C \
                   ForMani/Global/String.C
