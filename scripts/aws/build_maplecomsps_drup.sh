#!/bin/bash
set -e

cd /home/ubuntu/
mkdir -p MapleCOMSPS
cd MapleCOMSPS/
aws s3 cp s3://msoos-solve-data/solvers/MapleCOMSPS.tar.gz . --region=us-west-2
tar xzvf MapleCOMSPS.tar.gz
cd simp
MROOT=.. make clean rs
mv minisat_static maplecomsps_static

# binary is now at:
# --solver MapleCOMSPS/simp/maplecomsps_static

cd /home/ubuntu/
