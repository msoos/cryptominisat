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
        'ami-4b69577b',
        key_name='controlkey',
        instance_type='t1.micro',
        security_groups=['sg-507b3f35'],
        instance_initiated_shutdown_behaviour = 'terminate',
        instance_profile_arn = 'arn:aws:iam::907572138573:instance-profile/server',
        user_data=data)
