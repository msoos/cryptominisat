aws s3 cp s3://msoos-solve-data/solvers/SWDiA5BY.alt.vd.res.va2.15000.looseres.3tierC5.tar.gz . --region=us-west-2
tar zvf SWDiA5BY.alt.vd.res.va2.15000.looseres.3tierC5.tar.gz
cd SWDiA5BY.alt.vd.res.va2.15000.looseres.3tierC5
./build.sh

# now binary is at : /home/ubuntu/SWDiA5BY.alt.vd.res.va2.15000.looseres.3tierC5/binary/SWDiA5BY_static