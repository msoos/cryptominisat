PYTHON=python

pycosat.so: ../build/lib/libcryptominisat3.so pycryptosat.cpp
	$(PYTHON) setup.py build_ext --inplace

test: pycryptosat.so
	$(PYTHON) test_pycryptosat.py

clean:
	rm -rf build dist *.egg-info
	rm -f *.pyc *.so *.o *.a
