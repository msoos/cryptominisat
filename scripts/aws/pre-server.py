#!/usr/bin/env python
# -*- coding: utf-8 -*-

import boto.utils
import os

user_data = boto.utils.get_instance_userdata()

todo = ""
for line in user_data.split("\n"):
    if "DATA" in line:
        todo = line[5:].strip().strip('"')

if todo == "":
    exit(0)

os.chdir("/home/ubuntu/cryptominisat/scripts/aws/")
command = "nohup /home/ubuntu/cryptominisat/scripts/aws/server.py %s &" % todo
os.system(command)
