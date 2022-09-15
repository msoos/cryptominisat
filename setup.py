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
import setuptools
import sysconfig
from distutils.cmd import Command

def cleanup(dat):
    ret = []
    for elem in dat:
        elem = elem.strip()
        #if is_apple != "" and "-ldl" in elem:
            #continue
        if elem != "" and not "flto" in elem:
            ret.append(elem)

    return ret


def _init_posix(init):
    """
    Forces g++ instead of gcc on most systems
    credits to eric jones (eric@enthought.com) (found at Google Groups)
    """
    def wrapper(vars):
        init(vars)

        config_vars = sysconfig.get_config_vars()  # by reference
        if config_vars["MACHDEP"].startswith("sun"):
            # Sun needs forced gcc/g++ compilation
            config_vars['CC'] = 'gcc'
            config_vars['CXX'] = 'g++'

        config_vars['CFLAGS'] = '-g -W -Wall -Wno-deprecated'
        config_vars['OPT'] = '-g -W -Wall -Wno-deprecated'

    return wrapper

sysconfig._init_posix = _init_posix(sysconfig._init_posix)


class TestCommand(Command):
    """Call tests with the custom 'python setup.py test' command."""

    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):

        import os
        import glob
        print("our CWD is:", os.getcwd(), "files here: ", glob.glob("*"))
        sys.path.append(os.getcwd())
        path2 = os.path.join(os.getcwd(), "..")
        path2 = os.path.join(path2, "lib")
        path2 = os.path.normpath(path2)
        print("path2 is:", path2)
        sys.path.append(path2)
        print("our sys.path is", sys.path)

        import python.tests as tp
        return tp.run()


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


modules = setuptools.Extension(
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
    define_macros = [('CMS_LIBRARY_VERSION', '"5.11.0"')],
    extra_compile_args = ['-I../', '-Isrc/', '-std=c++17'],
    language = "c++",
)


if __name__ == '__main__':
    setuptools.setup(
        ext_modules =  [modules],
    #    py_modules = ['pycryptosat'],
        libraries = [picosatlib],
    )
