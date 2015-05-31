#!/usr/bin/env python
# -*- coding: utf-8 -*-

import boto.utils
import os

todo = boto.utils.get_instance_userdata()
if todo == "":
    return

command = "nohup /home/ubuntu/cryptominisat/scripts/aws/server.py %s &" % todo
os.system(command)
