#!/usr/bin/env python
# -*- coding: utf-8 -*-

import boto.utils
import os

todo = boto.utils.get_instance_userdata()
if todo == "":
    exit(0)

os.chdir("/home/ubuntu/cryptominisat/scripts/aws/")
command = "nohup /home/ubuntu/cryptominisat/scripts/aws/server.py %s &" % todo
os.system(command)
