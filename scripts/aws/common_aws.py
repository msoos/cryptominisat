#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import os
import time
import boto
import traceback
import sys
import subprocess
import socket
import fcntl
import struct
from email.mime.text import MIMEText
from email.mime.application import MIMEApplication
from email.mime.multipart import MIMEMultipart
import smtplib
import ConfigParser
config = ConfigParser.ConfigParser()
config.read("/home/ubuntu/email.conf")


def send_email(subject, text, fname = None):
    msg = MIMEMultipart()
    msg['Subject'] = 'Email from solver: %s' % subject
    msg['From'] = 'msoos@msoos.org'
    msg['To'] = 'soos.mate@gmail.com'

    # That is what you see if you have no email client:
    msg.preamble = 'Multipart massage.\n'

    # Text part
    part = MIMEText(text)
    msg.attach(part)

    # Attachment(s)
    if fname:
        part = MIMEApplication(open(fname,"rb").read())
        part.add_header('Content-Disposition', 'attachment', filename="attachment.txt")
        msg.attach(part)

    # Connect to STMP server
    email_login = config.get("email", "login")
    email_pass = config.get("email", "pass")

    smtp = smtplib.SMTP_SSL("email-smtp.us-west-2.amazonaws.com")
    smtp.login(email_login, email_pass)

    # Send email
    smtp.sendmail(msg['From'], msg['To'], msg.as_string())


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

        k.key = folder + "/" + "logs/" + fname
        boto_bucket.delete_key(k)
        k.set_contents_from_filename(logfile_name)

    except:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        the_trace = traceback.format_exc().rstrip().replace("\n", " || ")
        print("traceback for boto issue: %s" % the_trace)


def get_s3_folder(folder, rev, timeout, memout):
    print("folder: %s rev: %s tout: %s memout %s" % (folder, rev, timeout, memout))
    return folder + "-%s-tout-%d-mout-%d" \
        % (rev[:9],
           timeout,
           memout)
