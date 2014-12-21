#!/bin/bash
cd /home/ubuntu/cryptominisat
git stash
git checkout -f master
git pull
/home/ubuntu/cryptominisat/scripts/aws/client.py --host 172.30.0.242 > /home/ubuntu/log.txt &
