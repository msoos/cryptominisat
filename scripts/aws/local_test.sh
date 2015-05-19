set -e
./server.py --noaws --dir "/home/soos/development/sat_solvers/" -t 3 --extratime 2 --cnfdir todo &
./client.py --noaws --dir "/home/soos/development/sat_solvers/" --noshutdown --host localhost --temp /tmp/ --net lo --threads 2
wait