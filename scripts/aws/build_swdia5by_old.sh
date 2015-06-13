#!/bin/bash
set -e

cd /home/ubuntu/
mkdir -p SWDiA5BY_A26
cd SWDiA5BY_A26
aws s3 cp s3://msoos-solve-data/solvers/SWDiA5BY_A26.zip . --region=us-west-2
unzip SWDiA5BY_A26.zip
./build.sh

# binary is now at:
# --solver SWDiA5BY_A26/binary/SWDiA5BY_static

cd /home/ubuntu/
