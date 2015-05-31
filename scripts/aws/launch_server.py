#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import boto.ec2

data = " ".join(sys.argv[1:])
if len(sys.argv) > 1:
    print "Launching with data: '%s'" % data
else:
    print "Launching without any parameters"

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
