PYTHON=python

pycosat.so: libpicosat.a pycosat.c
	$(PYTHON) setup.py build_ext --inplace

picosat.o: picosat.c picosat.h
	$(CC) $(CFLAGS) -fPIC -c $<

libpicosat.a: picosat.o
	ar rc $@ picosat.o 


test: pycosat.so
	$(PYTHON) test_pycosat.py


clean:
	rm -rf build dist *.egg-info
	rm -f *.pyc *.so *.o *.a
