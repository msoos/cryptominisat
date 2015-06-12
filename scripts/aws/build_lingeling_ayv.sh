#!/bin/bash
set -e

cd /home/ubuntu
mkdir -p lingeling_ayv
cd lingeling_ayv
aws s3 cp s3://msoos-solve-data/solvers/lingeling-ayv-86bf266-140429.zip . --region=us-west-2
unzip lingeling-ayv-86bf266-140429.zip
./build.sh

#binary is now at:
# --solver lingeling_ayv/binary/lingeling

cd /home/ubuntu
