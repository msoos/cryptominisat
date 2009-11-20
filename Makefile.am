# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4

SUBDIRS = Solver SatElite

all-local: Solver SatElite
	cp Solver/cryptominisat .
	cp SatElite/satelite .

