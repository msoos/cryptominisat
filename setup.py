# -*- coding: utf-8 -*-
#
# CryptoMiniSat
#
# Copyright (c) 2009-2014, Mate Soos. All rights reserved.
#
#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in
#all copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#THE SOFTWARE.

from distutils.core import setup, Extension
from distutils import sysconfig

__PACKAGE_VERSION__ = "0.1.1"
__LIBRARY_VERSION__ = "5.0.1"


# Delete unwanted flags for C compilation
# Distutils has the lovely feature of providing all the same flags that
# Python was compiled with. The result is that adding extra flags is easy,
# but removing them is a total pain. Doing so involves subclassing the
# compiler class, catching the arguments and manually removing the offending
# flag from the argument list used by the compile function.
# That's the theory anyway, the docs are too poor to actually guide you
# through what you have to do to make that happen.
d = sysconfig.get_config_vars()
for k, v in d.items():
    for unwanted in ('-Wstrict-prototypes', '-DNDEBUG', ' -g ',
                     '-O2', '-D_FORTIFY_SOURCE=2', '-fstack-protector-strong'):
        if str(v).find(unwanted) != -1:
            v = d[k] = str(v).replace(unwanted, ' ')

################################################################################

def _init_posix(init):
    """
    Forces g++ instead of gcc on most systems
    credits to eric jones (eric@enthought.com) (found at Google Groups)
    """
    def wrapper():
        init()

        config_vars = sysconfig.get_config_vars()  # by reference
        if config_vars["MACHDEP"].startswith("sun"):
            # Sun needs forced gcc/g++ compilation
            config_vars['CC'] = 'gcc'
            config_vars['CXX'] = 'g++'

    return wrapper

sysconfig._init_posix = _init_posix(sysconfig._init_posix)

################################################################################

cryptoms_lib_files = [
    "GitSHA1.cpp",
    "cnf.cpp",
    "propengine.cpp",
    "varreplacer.cpp",
    "clausecleaner.cpp",
    "clauseusagestats.cpp",
    "prober.cpp",
    "occsimplifier.cpp",
    "subsumestrengthen.cpp",
    "clauseallocator.cpp",
    "sccfinder.cpp",
    "solverconf.cpp",
    "distillerallwithall.cpp",
    "distillerlongwithimpl.cpp",
    "str_impl_w_impl_stamp.cpp",
    "solutionextender.cpp",
    "completedetachreattacher.cpp",
    "searcher.cpp",
    "solver.cpp",
    "gatefinder.cpp",
    "sqlstats.cpp",
    "implcache.cpp",
    "stamp.cpp",
    "compfinder.cpp",
    "comphandler.cpp",
    "hyperengine.cpp",
    "subsumeimplicit.cpp",
    "cleaningstats.cpp",
    "datasync.cpp",
    "reducedb.cpp",
    "clausedumper.cpp",
    "bva.cpp",
    "intree.cpp",
    "features_calc.cpp",
    "features_to_reconf.cpp",
    "solvefeatures.cpp",
    "searchstats.cpp",
    "xorfinder.cpp",
    "cryptominisat_c.cpp",
    "cryptominisat.cpp",
#    "gaussian.cpp",
#    "matrixfinder.cpp",
]

modules = [
    Extension(
        "pycryptosat",
        ["python/pycryptosat.cpp"] + ['src/' + fd for fd in cryptoms_lib_files],
        language = "c++",
        swig_opts=['-c++', '-threads', '-includeall'],
        include_dirs=['src', 'cryptominisat5', '.'],
        extra_compile_args=[
            "-flto",
            "-march=native",
            "-mtune=native",
            "-Ofast",
            #"-O3",
            #"-Wall",
            # "-g", # Not define NDEBUG macro => Debug build
            "-DNDEBUG", # Force release build
            #"-DBOOST_TEST_DYN_LINK",
            #"-DUSE_ZLIB",
            "-std=c++11",
            "-Wno-unused-variable",
            "-Wno-unused-but-set-variable",
            # assume that signed arithmetic overflow of addition, subtraction
            # and multiplication wraps around using twos-complement
            # representation
            "-fwrapv",
            #BOF protect (use both)
            #"-D_FORTIFY_SOURCE=2",
            #"-fstack-protector-strong",
            "-pthread",
            "-DUSE_PTHREADS",
            "-fopenmp",
            "-D_GLIBCXX_PARALLEL",
        ],
        extra_link_args=[
            "-Ofast",
            "-march=native",
            "-flto",
            #"-lz",
            "-fopenmp",
        ]
    ),
]

setup(
    name = "pycryptosat",
    version = __LIBRARY_VERSION__,
    author = "Mate Soos",
    author_email = "soos.mate@gmail.com",
    url = "https://github.com/msoos/cryptominisat",
    license = "LGPLv2",
    classifiers = [
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Operating System :: OS Independent",
        "Programming Language :: C++",
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 2.5",
        "Programming Language :: Python :: 2.6",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
        "Topic :: Utilities",
    ],
    ext_modules = modules,
    py_modules = ['pycryptosat'],
    description = "bindings to CryptoMiniSat (a SAT solver)",
    long_description = open('python/README.rst').read(),
)
