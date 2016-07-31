#!/bin/bash
set -e

cd /home/ubuntu
git clone https://github.com/msoos/glucose2015.git
cd glucose2015
cd simp
make rs

#binary is now at:
# --solver glucose2015/simp/glucose_static

cd /home/ubuntu
