#!/bin/bash
set -e

cd /home/ubuntu
git clone https://github.com/msoos/drat-trim.git
cd drat-trim
make

#binary is now at:
# drat-trim/drat-trim2

cd /home/ubuntu
