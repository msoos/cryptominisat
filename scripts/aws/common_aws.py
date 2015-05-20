#!/usr/bin/env python
# -*- coding: utf-8 -*-

def try_upload_log_with_aws_cli():
    if options.noaws:
        return

    try:
        fname = "server-log-" + time.strftime("%c") + ".txt"
        fname = fname.replace(' ', '-')
        fname = fname.replace(':', '.')
        sendlog = "aws s3 cp %s s3://msoos-logs/%s" % (options.logfile_name,
                                                       fname)
        os.system(sendlog)
    except:
        pass
