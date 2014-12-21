#!/bin/bash
cd /home/ubuntu/cryptominisat
git stash
git checkout -f remotes/origin/master
git branch -D master
git checkout -b master
git branch --set-upstream-to=origin/master master
git pull origin master
