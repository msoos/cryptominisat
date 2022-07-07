all: clean compile sdist

clean:
	@echo Clean Python build dir...
	cd .. && python ./setup.py clean --all
	@echo Clean Python distribution dir...
	@-rm -rf ../dist

compile:
	@echo Building the library...
	cd .. && python setup.py build

test:
	@echo Test the solver...
	cd .. && python setup.py test

sdist:
	@echo Building the distribution package...
	cd .. && python setup.py sdist

install:
	@echo Install the package...
	cd .. && python setup.py install --record files.txt

uninstall: ../files.txt
	@echo Uninstalling the package...
	cat ../files.txt | xargs rm -rf
	rm ../files.txt

test_register:
	cd .. && python setup.py register -r https://testpypi.python.org/pypi

test_install:
	cd .. && python setup.py sdist upload -r https://testpypi.python.org/pypi
	pip install -U -i https://testpypi.python.org/pypi pycryptosat
