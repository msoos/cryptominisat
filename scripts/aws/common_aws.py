#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import time
import boto
import traceback


def try_upload_log_with_aws_cli(logfile_name, cli_or_server):
    try:
        boto_conn = boto.connect_s3()
        boto_bucket = boto_conn.get_bucket("msoos-logs")
        k = boto.s3.key.Key(boto_bucket)

        s3_folder = time.strftime("%c")
        s3_folder = s3_folder.replace(' ', '-')
        s3_folder = s3_folder.replace(':', '.')
        s3_folder = s3_folder.replace(',', '.')

        k.key = s3_folder + "/" + cli_or_server
        boto_bucket.delete_key(k)
        k.set_contents_from_filename(logfile_name)

    except:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        the_trace = traceback.format_exc().rstrip().replace("\n", " || ")
        print "traceback for boto issue:", the_trace
