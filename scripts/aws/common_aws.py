#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import time
import boto
import traceback
import sys
import subprocess
import socket
import fcntl
import struct


def get_ip_address(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return socket.inet_ntoa(fcntl.ioctl(
        s.fileno(),
        0x8915,  # SIOCGIFADDR
        struct.pack('256s', ifname[:15])
    )[20:24])


def get_revision(full_solver_path, base_dir):
    _, solvername = os.path.split(full_solver_path)
    if solvername == "cryptominisat":
        os.chdir('%s/cryptominisat' % base_dir)
        revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'])
    else:
        revision = solvername

    return revision.strip()


def upload_log(bucket, folder, logfile_name, fname):
    try:
        boto_conn = boto.connect_s3()
        boto_bucket = boto_conn.get_bucket(bucket)
        k = boto.s3.key.Key(boto_bucket)

        k.key = folder + "/" + fname
        boto_bucket.delete_key(k)
        k.set_contents_from_filename(logfile_name)

    except:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        the_trace = traceback.format_exc().rstrip().replace("\n", " || ")
        print "traceback for boto issue:", the_trace


def get_s3_folder(folder, rev, timeout, memout):
    s3_folder_ending = folder + "-%s-tout-%d-mout-%d" \
        % (rev[:6],
           timeout,
           memout)
