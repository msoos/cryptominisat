#!/bin/bash

# Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

# This file wraps CMake invocation for TravisCI
# so we can set different configurations via environment variables.

export FNAMEOUT="mydata"
export FIXED="3000"
export DUMPRATIO="0.1"
export CLLOCK="0.3"
export EXTRA_GEN_PANDAS_OPTS=""
export cut1="3.0"
export cut2="25.0"
export SANITIZE=0
export bestf="../../scripts/crystal/best_features-correlation2.txt"
export NOBUF=""
export NOBUF="stdbuf -oL -eL "
#export bestf="../../scripts/crystal/best_features-ext.txt"
