#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import boto.ec2


def get_answer():
    yes = set(['yes', 'y', 'ye', ''])
    no = set(['no', 'n'])

    choice = raw_input().lower()
    if choice in yes:
        return True
    elif choice in no:
        return False
    else:
        sys.stdout.write("Please respond with 'yes' or 'no'\n")
        exit(0)

data = " ".join(sys.argv[1:])
if len(sys.argv) > 1:
    print "Launching with data: %s" % data
else:
    print "Launching without any parameters"

sys.stdout.write("Is this OK? [y/n]? ")
if not get_answer():
    print "Aborting"
    exit(0)

print "Executing!"

conn = boto.ec2.connect_to_region("us-west-2")
conn.run_instances(
        min_count = 1,
        max_count = 1,
        image_id = 'ami-4b69577b',
        subnet_id = "subnet-88ab16ed",
        instance_type='t2.micro',
        instance_profile_arn = 'arn:aws:iam::907572138573:instance-profile/server',
        user_data=data,
        key_name='controlkey',
        security_group_ids=['sg-507b3f35'],
        instance_initiated_shutdown_behavior = 'terminate')
