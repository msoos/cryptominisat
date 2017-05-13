#!/bin/bash
set -e

cd /home/ubuntu/

aws s3 cp s3://msoos-solve-data/solvers/lingeling-bbc.tar.gz . --region=us-west-2
tar xzvf lingeling-bbc.tar.gz
cd lingeling-bbc/build
sh build.sh
mv lingeling/lingeling lingeling/lingeling_bbc

# lingeling is now in
# --solver lingeling-bbc/build/lingeling/lingeling_bbc

cd /home/ubuntu/
