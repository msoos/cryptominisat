import sys
from distutils.core import setup, Extension


version = '0.6.0.dev'


ext_kwds = dict(
    name = "pycosat",
    sources = ["pycosat.c"],
    define_macros = []
)
if sys.platform != 'win32':
    ext_kwds['define_macros'].append(('PYCOSAT_VERSION', '"%s"' % version))
if '--inplace' in sys.argv:
    ext_kwds['define_macros'].append(('DONT_INCLUDE_PICOSAT', 1))
    ext_kwds['library_dirs'] = ['.']
    ext_kwds['libraries'] = ['picosat']


setup(
    name = "pycosat",
    version = version,
    author = "Ilan Schnell",
    author_email = "ilan@continuum.io",
    url = "https://github.com/ContinuumIO/pycosat",
    license = "MIT",
    classifiers = [
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Operating System :: OS Independent",
        "Programming Language :: C",
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 2.5",
        "Programming Language :: Python :: 2.6",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.2",
        "Programming Language :: Python :: 3.3",
        "Topic :: Utilities",
    ],
    ext_modules = [Extension(**ext_kwds)],
    py_modules = ['test_pycosat'],
    description = "bindings to picosat (a SAT solver)",
    long_description = open('README.rst').read(),
)
