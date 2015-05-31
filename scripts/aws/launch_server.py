#!/usr/bin/env python
# -*- coding: utf-8 -*-

import boto.ec2
conn = boto.ec2.connect_to_region("us-west-2")
conn.run_instances(
        '<ami-image-id>',
        key_name='controlkey',
        instance_type='t1.micro',
        security_groups=['sg-507b3f35'],
        instance_initiated_shutdown_behaviour = 'terminate',
        instance_profile_arn = 'arn:aws:iam::907572138573:instance-profile/server')
