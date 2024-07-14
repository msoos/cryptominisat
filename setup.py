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
from setuptools import Extension, setup
import sysconfig
import tomli as toml
import pathlib
from sys import platform

def _parse_toml(pyproject_path):
    pyproject_text = pyproject_path.read_text()
    pyproject_data = toml.loads(pyproject_text)
    return pyproject_data['project']['version']

picosatlib = ('picosatlib', {
    'sources': [
               "src/mpicosat/mpicosat.c",
               "src/mpicosat/version.c"],
    'language' : "c",
    'define_macros' : [("TRACE", "ON")],
    'include_dirs' : ["src/mpicosat/"]
    })


def gen_modules(version):

    if platform == "win32" or platform == "cygwin":
        extra_compile_args_val = ['-I../', '-Isrc/', '/std:c++17', "/DCMS_FULL_VERSION=\""+version+"\""]
        define_macros_val = [("TRACE", "")]

    else:
        extra_compile_args_val = ['-I../', '-Isrc/', '-std=c++17']
        define_macros_val = [("TRACE", ""), ("CMS_FULL_VERSION", "\""+version+"\"")]

    modules = Extension(
        name = "pycryptosat",
        include_dirs = ["src/"],
        sources = ["python/src/pycryptosat.cpp",
                   "python/src/GitSHA1.cpp",
                   "python/cadical/src/analyze.cpp",
                   "python/cadical/src/arena.cpp",
                   "python/cadical/src/assume.cpp",
                   "python/cadical/src/averages.cpp",
                   "python/cadical/src/backtrack.cpp",
                   "python/cadical/src/backward.cpp",
                   "python/cadical/src/bins.cpp",
                   "python/cadical/src/block.cpp",
                   "python/cadical/src/checker.cpp",
                   "python/cadical/src/clause.cpp",
                   "python/cadical/src/collect.cpp",
                   "python/cadical/src/compact.cpp",
                   "python/cadical/src/condition.cpp",
                   "python/cadical/src/config.cpp",
                   "python/cadical/src/constrain.cpp",
                   "python/cadical/src/contract.cpp",
                   "python/cadical/src/cover.cpp",
                   "python/cadical/src/decide.cpp",
                   "python/cadical/src/decompose.cpp",
                   "python/cadical/src/deduplicate.cpp",
                   "python/cadical/src/drattracer.cpp",
                   "python/cadical/src/elim.cpp",
                   "python/cadical/src/ema.cpp",
                   "python/cadical/src/extend.cpp",
                   "python/cadical/src/external.cpp",
                   "python/cadical/src/external_propagate.cpp",
                   "python/cadical/src/file.cpp",
                   "python/cadical/src/flags.cpp",
                   "python/cadical/src/flip.cpp",
                   "python/cadical/src/format.cpp",
                   "python/cadical/src/frattracer.cpp",
                   "python/cadical/src/gates.cpp",
                   "python/cadical/src/idruptracer.cpp",
                   "python/cadical/src/instantiate.cpp",
                   "python/cadical/src/internal.cpp",
                   "python/cadical/src/ipasir.cpp",
                   "python/cadical/src/lidruptracer.cpp",
                   "python/cadical/src/limit.cpp",
                   "python/cadical/src/logging.cpp",
                   "python/cadical/src/lookahead.cpp",
                   "python/cadical/src/lratbuilder.cpp",
                   "python/cadical/src/lratchecker.cpp",
                   "python/cadical/src/lrattracer.cpp",
                   "python/cadical/src/lucky.cpp",
                   "python/cadical/src/message.cpp",
                   "python/cadical/src/minimize.cpp",
                   "python/cadical/src/mobical.cpp",
                   "python/cadical/src/occs.cpp",
                   "python/cadical/src/options.cpp",
                   "python/cadical/src/parse.cpp",
                   "python/cadical/src/phases.cpp",
                   "python/cadical/src/probe.cpp",
                   "python/cadical/src/profile.cpp",
                   "python/cadical/src/proof.cpp",
                   "python/cadical/src/propagate.cpp",
                   "python/cadical/src/queue.cpp",
                   "python/cadical/src/random.cpp",
                   "python/cadical/src/reap.cpp",
                   "python/cadical/src/reduce.cpp",
                   "python/cadical/src/rephase.cpp",
                   "python/cadical/src/report.cpp",
                   "python/cadical/src/resources.cpp",
                   "python/cadical/src/restart.cpp",
                   "python/cadical/src/restore.cpp",
                   "python/cadical/src/score.cpp",
                   "python/cadical/src/shrink.cpp",
                   "python/cadical/src/signal.cpp",
                   "python/cadical/src/solution.cpp",
                   "python/cadical/src/solver.cpp",
                   "python/cadical/src/stats.cpp",
                   "python/cadical/src/subsume.cpp",
                   "python/cadical/src/terminal.cpp",
                   "python/cadical/src/ternary.cpp",
                   "python/cadical/src/transred.cpp",
                   "python/cadical/src/util.cpp",
                   "python/cadical/src/var.cpp",
                   "python/cadical/src/veripbtracer.cpp",
                   "python/cadical/src/version.cpp",
                   "python/cadical/src/vivify.cpp",
                   "python/cadical/src/walk.cpp",
                   "python/cadical/src/watch.cpp",
                   "python/cadiback/cadiback.cpp",
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
        extra_compile_args = extra_compile_args_val,
        define_macros=define_macros_val,
        language = "c++",
    )
    return modules

if __name__ == '__main__':
    pyproject_path = pathlib.Path('pyproject.toml')
    version = _parse_toml(pyproject_path)
    modules = gen_modules(version)
    setup(
        ext_modules =  [modules],
        libraries = [picosatlib],
    )
