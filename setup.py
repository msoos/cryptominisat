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
from distutils.core import setup, Extension
import sysconfig
from distutils.cmd import Command

__PACKAGE_VERSION__ = "1.0.0"
__LIBRARY_VERSION__ = "5.9.0"
#os.environ["CC"] = "${CMAKE_C_COMPILER}"
#os.environ["CXX"] = "${CMAKE_CXX_COMPILER}"

#cconf = """${PY_C_CONFIG}""".split(" ")
#is_apple = """${APPLE}"""


def cleanup(dat):
    ret = []
    for elem in dat:
        elem = elem.strip()
        #if is_apple != "" and "-ldl" in elem:
            #continue
        if elem != "" and not "flto" in elem:
            ret.append(elem)

    return ret

#cconf = cleanup(cconf)
# print "Extra C flags from python-config:", cconf


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


__version__ = '@PROJECT_VERSION@'

# needed because Mac doesn't make use of runtime_library_dirs
extra_link_args = []
if platform.system() == 'Darwin':
    extra_link_args.append('-Wl,-rpath,')
    # NOTE: below apparently could be obtained via: "xcrun --show-sdk-path"


myclib = ('myclib', {'sources': [
               ### picosat ####
               "src/picosat/app.c",
               "src/picosat/picogcnf.c",
               "src/picosat/picomcs.c",
               "src/picosat/picomus.c",
               "src/picosat/picosat.c",
               "src/picosat/version.c"],
    'language' : "c",
    'extra_compile_args' : [],
    })


modules = dict(
    name = "pycryptosat",
    sources = ["python/src/pycryptosat.cpp",
               "python/src/GitSHA1.cpp",
               "src/bva.cpp",
               "src/cardfinder.cpp",
               "src/ccnr_cms.cpp",
               "src/ccnr.cpp",
               "src/clauseallocator.cpp",
               "src/clausecleaner.cpp",
               #"src/cl_predictors_abs.cpp",
               #"src/cl_predictors_lgbm.cpp",
               #"src/cl_predictors_py.cpp",
               #"src/cl_predictors_xgb.cpp",
               #"src/cms_bosphorus.cpp",
               #"src/cms_breakid.cpp",
               "src/cnf.cpp",
               #"src/community_finder.cpp",
               "src/completedetachreattacher.cpp",
               "src/cryptominisat_c.cpp",
               "src/cryptominisat.cpp",
               "src/datasync.cpp",
               #"src/datasyncserver.cpp",
               "src/distillerbin.cpp",
               "src/distillerlitrem.cpp",
               "src/distillerlong.cpp",
               "src/distillerlongwithimpl.cpp",
               "src/frat.cpp",
               #"src/fuzz.cpp",
               "src/gatefinder.cpp",
               "src/gaussian.cpp",
               "src/get_clause_query.cpp",
               "src/hyperengine.cpp",
               "src/intree.cpp",
               #"src/ipasir.cpp",
               "src/lucky.cpp",
               "src/matrixfinder.cpp",
               "src/occsimplifier.cpp",
               "src/packedrow.cpp",
               "src/propengine.cpp",
               "src/reducedb.cpp",
               "src/satzilla_features_calc.cpp",
               "src/satzilla_features.cpp",
               "src/sccfinder.cpp",
               "src/searcher.cpp",
               "src/searchstats.cpp",
               #"src/simple.cpp",
               "src/sls.cpp",
               "src/solutionextender.cpp",
               "src/solverconf.cpp",
               "src/solver.cpp",
               "src/str_impl_w_impl.cpp",
               "src/subsumeimplicit.cpp",
               "src/subsumestrengthen.cpp",
               #"src/toplevelgauss.cpp",
               "src/vardistgen.cpp",
               "src/varreplacer.cpp",
               "src/xorfinder.cpp",
               ### oracle ###
               "src/oracle/oracle.cpp",
               ],
    define_macros = [('LIBRARY_VERSION', '"' + __LIBRARY_VERSION__ + '"')],
    extra_compile_args = ['-I../', '-Isrc/', '-std=c++17'],
    #extra_link_args = extra_link_args,
    language = "c++",
    #library_dirs=['.', '${PROJECT_BINARY_DIR}/lib', '${PROJECT_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE}'],
    #runtime_library_dirs=['${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}'],
    #libraries = []
)

if platform.system() == 'Windows':
    del modules['runtime_library_dirs']

setup(
    name = "pycryptosat",
    version = __PACKAGE_VERSION__,
    author = "Mate Soos",
    author_email = "soos.mate@gmail.com",
    url = "https://github.com/msoos/cryptominisat",
    license = "MIT",
    classifiers = [
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Operating System :: OS Independent",
        "Programming Language :: C++",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.5",
        "License :: OSI Approved :: MIT License",
        "Topic :: Utilities",
    ],
    ext_modules =  [Extension(**modules)],
    description = "Bindings to CryptoMiniSat {} (a SAT solver)".\
        format(__LIBRARY_VERSION__),
#    py_modules = ['pycryptosat'],
    libraries = [myclib],
    long_description = open('python/README.rst').read(),
    cmdclass={
        'test': TestCommand
    }

)

