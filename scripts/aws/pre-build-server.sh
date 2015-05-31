#!/bin/bash
cd /home/ubuntu/cryptominisat
git stash
git checkout -f remotes/origin/master
git branch -D master
git checkout -b master
git branch --set-upstream-to=origin/master master
git pull origin master
/home/ubuntu/cryptominisat/scripts/aws/pre-server.py > /home/ubuntu/pre_server_log.txt  2>&1 &
