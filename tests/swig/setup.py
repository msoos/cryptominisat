import sys
from distutils.core import setup, Extension
from distutils import sysconfig

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

        # FIXME raises hardening-no-fortify-functions lintian warning.
        else:
            # Non-Sun needs linkage with g++
            config_vars['LDSHARED'] = 'g++ -shared -g -W -Wall -Wno-deprecated'

        config_vars['CFLAGS'] = '-g -W -Wall -Wno-deprecated'
        config_vars['OPT'] = '-g -W -Wall -Wno-deprecated'

    return wrapper

sysconfig._init_posix = _init_posix(sysconfig._init_posix)

ext_kwds = dict(
    name = "_swigdemo",
    sources = ["_swigdemo_module.cc"],
    library_dirs=["/home/soos/development/sat_solvers/cryptominisat/build/lib"],
    libraries=["cryptominisat4"],
    extra_compile_args = ['-I/usr/include/python2.7 -I/usr/include/x86_64-linux-gnu/python2.7  -fno-strict-aliasing -D_FORTIFY_SOURCE=2 -g -fstack-protector-strong -Wformat -Werror=format-security  -DNDEBUG -g -fwrapv -O2 -Wall -Wstrict-prototypes', '-std=c++11', '-I../build/cmsat4-src'],
    extra_link_args = ["-L/usr/lib/python2.7/config-x86_64-linux-gnu -L/usr/lib -lpython2.7 -lpthread -ldl  -lutil -lm  -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions"],
    runtime_library_dirs = ["/home/soos/development/sat_solvers/cryptominisat/build/lib"]
)

extension_mod = Extension(**ext_kwds)

setup(name = "swigdemo", ext_modules=[extension_mod])
