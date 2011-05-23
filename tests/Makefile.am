METASOURCES = AUTO

#TESTS=
EXTRA_DIST = AProVE09-12.cnf.gz test.sh

check-local:
	cp $(buildddir)../cryptominisat .
	chmod +x $(srcdir)/test.sh
	$(srcdir)/test.sh
