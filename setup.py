#
# CryptoMiniSat
#
# Copyright (c) 2009-2017, Mate Soos. All rights reserved.
# Copyright (c) 2017, Pierre Vignet
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.


import sys
import os
import platform
from setuptools import Extension, setup
import sysconfig


picosatlib = ('picosatlib', {
    'sources': [
               "src/picosat/picogcnf.c",
               "src/picosat/picomcs.c",
               "src/picosat/picomus.c",
               "src/picosat/picosat.c",
               "src/picosat/version.c"],
    'language' : "c",
    'extra_compile_args' : [],
    'include_dirs' : ["src/picosat/"]
    })


modules = Extension(
    name = "pycryptosat",
    include_dirs = ["src/"],
    sources = ["python/src/pycryptosat.cpp",
               "python/src/GitSHA1.cpp",
               "src/bva.cpp",
               "src/cardfinder.cpp",
               "src/ccnr_cms.cpp",
               "src/ccnr.cpp",
                   "src/clauseallocator.cpp",
               "src/clausecleaner.cpp",
               "src/cnf.cpp",
               "src/completedetachreattacher.cpp",
               "src/cryptominisat_c.cpp",
               "src/cryptominisat.cpp",
               "src/datasync.cpp",
               "src/distillerbin.cpp",
               "src/distillerlitrem.cpp",
               "src/distillerlong.cpp",
               "src/distillerlongwithimpl.cpp",
               "src/frat.cpp",
               "src/gatefinder.cpp",
               "src/gaussian.cpp",
               "src/get_clause_query.cpp",
               "src/hyperengine.cpp",
               "src/intree.cpp",
               "src/lucky.cpp",
               "src/matrixfinder.cpp",
               "src/occsimplifier.cpp",
               "src/packedrow.cpp",
               "src/propengine.cpp",
               "src/reducedb.cpp",
               "src/sccfinder.cpp",
               "src/searcher.cpp",
               "src/searchstats.cpp",
               "src/sls.cpp",
               "src/solutionextender.cpp",
               "src/solverconf.cpp",
               "src/solver.cpp",
               "src/str_impl_w_impl.cpp",
               "src/subsumeimplicit.cpp",
               "src/subsumestrengthen.cpp",
               "src/varreplacer.cpp",
               "src/xorfinder.cpp",
               "src/oracle/oracle.cpp",
           ],
    extra_compile_args = ['-I../', '-Isrc/', '-std=c++17'],
    language = "c++",
)

if __name__ == '__main__':
    setup(
        ext_modules =  [modules],
        libraries = [picosatlib],
    )
