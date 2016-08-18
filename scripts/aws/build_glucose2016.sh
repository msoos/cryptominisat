#!/bin/bash
set -e

cd /home/ubuntu
git clone https://github.com/msoos/glucose2016.git
cd glucose2016
cd simp
make rs

#binary is now at:
# --solver glucose2016/simp/glucose_static

cd /home/ubuntu
