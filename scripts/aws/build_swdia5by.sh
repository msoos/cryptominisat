#!/bin/bash
set -e

cd /home/ubuntu/
aws s3 cp s3://msoos-solve-data/solvers/SWDiA5BY.alt.vd.res.va2.15000.looseres.3tierC5.tar.gz . --region=us-west-2
tar xzvf SWDiA5BY.alt.vd.res.va2.15000.looseres.3tierC5.tar.gz
cd SWDiA5BY.alt.vd.res.va2.15000.looseres.3tierC5
./build.sh

# binary is now at:
# --solver SWDiA5BY.alt.vd.res.va2.15000.looseres.3tierC5/binary/SWDiA5BY_static

cd /home/ubuntu/
