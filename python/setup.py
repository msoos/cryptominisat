import sys
from distutils.core import setup, Extension


version = '3.4.0.dev'


ext_kwds = dict(
    name = "pycryptosat",
    sources = ["pycryptosat.cpp"],
    define_macros = [],
    #extra_compile_args = ['--std=c++11'],
    library_dirs=['.', '/usr/local/lib'],
    libraries = ['cryptominisat3']
)


setup(
    name = "pycryptosat",
    version = version,
    author = "Mate Soos (adapted from code by Ilan Schnell)",
    author_email = "soos.mate@gmail.com",
    url = "https://github.com/msoos/cryptominisat",
    license = "LGPL",
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
    py_modules = ['test_pycryptosat'],
    description = "bindings to CryptoMiniSat (a SAT solver)",
    long_description = open('README.rst').read(),
)
