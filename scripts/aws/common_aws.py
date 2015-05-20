#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import time


def try_upload_log_with_aws_cli(logfile_name, systemtype):
    try:
        fname = systemtype + "-log-" + time.strftime("%c") + ".txt"
        fname = fname.replace(' ', '-')
        fname = fname.replace(':', '.')
        sendlog = "aws s3 cp %s s3://msoos-logs/%s" % (logfile_name,
                                                       fname)
        os.system(sendlog)
    except:
        pass
