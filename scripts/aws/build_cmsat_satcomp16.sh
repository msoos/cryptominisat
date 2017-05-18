#!/bin/bash
set -e

cd /home/ubuntu/
aws s3 cp s3://msoos-solve-data/solvers/cmsat-satcomp16.tar.gz . --region=us-west-2
tar xzvf cmsat-satcomp16.tar.gz
cd cmsat-satcomp16
./starexec_build

# binary is now at:
# --solver cmsat-satcomp16/bin/cryptominisat4_simple

cd /home/ubuntu/
