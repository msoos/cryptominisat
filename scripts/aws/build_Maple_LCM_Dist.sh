#!/bin/bash
set -e

cd /home/ubuntu
git clone https://github.com/msoos/Maple_LCM_Dist
cd Maple_LCM_Dist
./starexec_build.sh

#binary is now at:
# Maple_LCM_Dist/Maple_LCM_Dist

cd /home/ubuntu
