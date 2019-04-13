# Copyright (c) 2014 SiegeLord -- Pavel Sountsov
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
#    1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
#
#    2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
#
#    3. This notice may not be removed or altered from any source
#    distribution.

find_program(RUSTC_EXECUTABLE rustc)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(rustc DEFAULT_MSG RUSTC_EXECUTABLE)
mark_as_advanced(RUSTC_EXECUTABLE)

execute_process(COMMAND ${RUSTC_EXECUTABLE} -Vv
                OUTPUT_VARIABLE RUSTC_TARGET_TRIPLE
                OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "host:[ \t](.*)\n" RUSTC_TARGET_TRIPLE "${RUSTC_TARGET_TRIPLE}")
string(REGEX REPLACE "host:[ \t](.*)\n" "\\1" RUSTC_TARGET_TRIPLE "${RUSTC_TARGET_TRIPLE}")
set(RUSTC_TARGET_TRIPLE "${RUSTC_TARGET_TRIPLE}" CACHE STRING "Target triple you can pass to rustc (not passed by default)")
mark_as_advanced(RUSTC_TARGET_TRIPLE)
