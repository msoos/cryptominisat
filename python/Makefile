PYTHON=python

pycryptosat.so: pycryptosat.cpp
	$(PYTHON) setup.py build_ext --inplace

test: pycryptosat.so
	$(PYTHON) test_pycryptosat.py

install: pycryptosat.so
	$(PYTHON) setup.py install --record files.txt

uninstall: pycryptosat.so
	cat files.txt | xargs rm

clean:
	rm -rf build dist *.egg-info
	rm -f *.pyc *.so *.o *.a
