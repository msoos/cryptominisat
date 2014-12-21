#!/bin/bash
cd /home/ubuntu/cryptominisat
git pull
/home/ubuntu/cryptominisat/scripts/aws/client.py --host 172.30.0.242 > /home/ubuntu/log.txt &
