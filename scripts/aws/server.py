#!/usr/bin/env python
# -*- coding: utf-8 -*-

import random
import os
import socket
import sys
import optparse
import struct
import pickle
import time
import pprint
import traceback
import subprocess
import Queue
import threading
import logging

#for importing in systems where "." is not in the PATH
import glob
sys.path.append(os.getcwd())
from common_aws import *
import requester


last_termination_sent = None


def set_up_logging():
    form = '[ %(asctime)-15s  %(levelname)s  %(message)s ]'

    logformatter = logging.Formatter(form)

    try:
        os.unlink(options.logfile_name)
    except:
        pass
    fileHandler = logging.FileHandler(options.logfile_name)
    fileHandler.setFormatter(logformatter)
    logging.getLogger().addHandler(fileHandler)

    logging.getLogger().setLevel(logging.INFO)


class PlainHelpFormatter(optparse.IndentedHelpFormatter):

    def format_description(self, description):
        if description:
            return description + "\n"
        else:
            return ""

usage = "usage: %prog"
parser = optparse.OptionParser(usage=usage, formatter=PlainHelpFormatter())
parser.add_option("--verbose", "-v", action="store_true",
                  default=False, dest="verbose", help="Be more verbose"
                  )

parser.add_option("--port", "-p", default=10000, dest="port",
                  help="Port to listen on. [default: %default]", type="int"
                  )

parser.add_option("--tout", "-t", default=3600, dest="timeout_in_secs",
                  help="Timeout for the file in seconds"
                  "[default: %default]",
                  type=int
                  )

parser.add_option("--extratime", default=7 * 60, dest="extra_time",
                  help="Timeout for the server to send us the results"
                  "[default: %default]",
                  type=int
                  )

parser.add_option("--memlimit", "-m", default=1600, dest="mem_limit_in_mb",
                  help="Memory limit in MB"
                  "[default: %default]",
                  type=int
                  )

parser.add_option("--cnfdir", default="satcomp14", dest="cnf_dir_name",
                  type=str,
                  help="The list of CNF files to solve, first line the dir"
                  "[default: %default]",
                  )

parser.add_option("--dir", default="/home/ubuntu/", dest="base_dir", type=str,
                  help="The home dir of cryptominisat"
                  )

parser.add_option("--solver",
                  default="cryptominisat/build/cryptominisat",
                  dest="solver",
                  help="Solver executable"
                  "[default: %default]",
                  type=str
                  )

parser.add_option("--s3bucket", default="msoos-solve-results",
                  dest="s3_bucket", help="S3 Bucket to upload finished data"
                  "[default: %default]",
                  type=str
                  )

parser.add_option("--s3folder", default="results", dest="s3_folder",
                  help="S3 folder name to upload data"
                  "[default: %default]",
                  type=str
                  )

parser.add_option("--git", dest="git_rev", type=str,
                  help="The GIT revision to use. Default: HEAD"
                  )

parser.add_option("--opt", dest="extra_opts", type=str, default="",
                  help="Extra options to give to solver"
                  "[default: %default]",
                  )

parser.add_option("--noshutdown", "-n", default=False, dest="noshutdown",
                  action="store_true", help="Do not shut down clients"
                  )

parser.add_option("--noaws", default=False, dest="noaws",
                  action="store_true", help="Use AWS"
                  )

parser.add_option("--logfile", dest="logfile_name", type=str,
                  default="python_server_log.txt", help="Name of LOG file")


# parse options
(options, args) = parser.parse_args()

options.logfile_name = options.base_dir + options.logfile_name
options.s3_folder += "-" + time.strftime("%d-%B-%Y")
options.s3_folder += "-mark-%d" % int(random.randint(0, 1000))


def get_revision():
    _, solvername = os.path.split(options.base_dir + options.solver)
    if solvername != "cryptominisat":
        return solvername

    pwd = os.getcwd()
    os.chdir(options.base_dir + "cryptominisat")
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'])
    os.chdir(pwd)
    return revision.strip()


def get_n_bytes_from_connection(sock, MSGLEN):
    chunks = []
    bytes_recd = 0
    while bytes_recd < MSGLEN:
        chunk = sock.recv(min(MSGLEN - bytes_recd, 2048))
        if chunk == '':
            raise RuntimeError("socket connection broken")
        chunks.append(chunk)
        bytes_recd = bytes_recd + len(chunk)

    return ''.join(chunks)


class ToSolve:

    def __init__(self, num, name):
        self.num = num
        self.name = name

    def __str__(self):
        return "%s (num: %d)" % (self.name, self.num)

global acc_queue
acc_queue = Queue.Queue()


class Server (threading.Thread):

    def __init__(self):
        threading.Thread.__init__(self)
        self.files_available = []
        self.files_finished = []
        self.files = {}

        fnames = open(options.cnf_dir_name, "r")
        options.cnf_dir = fnames.next().strip()
        logging.info("CNF dir really is %s", options.cnf_dir)
        for num, fname in zip(xrange(10000), fnames):
            fname = fname.strip()
            self.files[num] = ToSolve(num, fname)
            self.files_available.append(num)
            logging.info("File added: %s", fname)
        fnames.close()

        self.files_running = {}
        logging.info("Solving %d files", len(self.files_available))
        self.uniq_cnt = 0

    def ready_to_shutdown(self):
        if len(self.files_available) > 0:
            return False

        if len(self.files_finished) < len(self.files):
            return False

        return True

    def handle_done(self, connection, cli_addr, indata):
        file_num = indata["file_num"]

        logging.info("Finished with file %s (num %d)",
                     self.files[indata["file_num"]], indata["file_num"])
        self.files_finished.append(indata["file_num"])
        if file_num in self.files_running:
            del self.files_running[file_num]

        logging.info("Num files_available: %d Num files_finished %d",
                     len(self.files_available), len(self.files_finished))
        sys.stdout.flush()

    def check_for_dead_files(self):
        this_time = time.time()
        files_to_remove_from_files_running = []
        for file_num, starttime in self.files_running.iteritems():
            duration = this_time - starttime
            # print "* death check. running:" , file_num, " duration: ",
            # duration
            if duration > options.timeout_in_secs + options.extra_time:
                logging.warn("* dead file %s duration: %d re-inserting",
                             file_num, duration)
                files_to_remove_from_files_running.append(file_num)
                self.files_available.append(file_num)

        for c in files_to_remove_from_files_running:
            del self.files_running[c]

    def find_something_to_solve(self):
        self.check_for_dead_files()
        logging.info("Num files_available pre-send: %d",
                     len(self.files_available))

        if len(self.files_available) == 0:
            return None

        file_num = self.files_available[0]
        del self.files_available[0]
        logging.info("Num files_available post-send: %d",
                     len(self.files_available))
        sys.stdout.flush()

        return file_num

    def handle_build(self, connection, cli_addr, indata):
        tosend = {}
        tosend["solver"] = options.base_dir + options.solver
        tosend["git_rev"] = options.git_rev
        tosend["s3_bucket"] = options.s3_bucket
        tosend["s3_folder"] = options.s3_folder
        tosend["timeout_in_secs"] = options.timeout_in_secs
        tosend["mem_limit_in_mb"] = options.mem_limit_in_mb
        tosend["noshutdown"] = options.noshutdown
        tosend = pickle.dumps(tosend)
        tosend = struct.pack('!q', len(tosend)) + tosend

        logging.info("Sending git revision %s to %s", options.git_rev,
                     cli_addr)
        connection.sendall(tosend)

    def handle_need(self, connection, cli_addr, indata):
        # TODO don't ignore 'indata' for solving CNF instances, use it to
        # opitimize for uptime
        file_num = self.find_something_to_solve()

        # yay, everything finished!
        if file_num is None:
            tosend = {}
            tosend["noshutdown"] = options.noshutdown
            tosend["command"] = "finish"
            tosend = pickle.dumps(tosend)
            tosend = struct.pack('!q', len(tosend)) + tosend

            logging.warn("No more to solve, sending termination to %s",
                         cli_addr)
            connection.sendall(tosend)
            global last_termination_sent
            last_termination_sent = time.time()
        else:
            # set timer that we have sent this to be solved
            self.files_running[file_num] = time.time()
            filename = self.files[file_num].name

            tosend = {}
            tosend["file_num"] = file_num
            tosend["git_rev"] = options.git_rev
            tosend["cnf_filename"] = filename
            tosend["solver"] = options.base_dir + options.solver
            tosend["timeout_in_secs"] = options.timeout_in_secs
            tosend["mem_limit_in_mb"] = options.mem_limit_in_mb
            tosend["s3_bucket"] = options.s3_bucket
            tosend["s3_folder"] = options.s3_folder
            tosend["cnf_dir"] = options.cnf_dir
            tosend["noshutdown"] = options.noshutdown
            tosend["extra_opts"] = options.extra_opts
            tosend["uniq_cnt"] = str(self.uniq_cnt)
            tosend["command"] = "solve"
            tosend = pickle.dumps(tosend)
            tosend = struct.pack('!q', len(tosend)) + tosend

            logging.info("Sending file %s (num %d) to %s",
                         filename, file_num, cli_addr)
            sys.stdout.flush()
            connection.sendall(tosend)
            self.uniq_cnt += 1

    def handle_one_client(self, conn, cli_addr):
        try:
            logging.info("connection from %s", cli_addr)

            data = get_n_bytes_from_connection(conn, 8)
            length = struct.unpack('!q', data)[0]
            data = get_n_bytes_from_connection(conn, length)
            data = pickle.loads(data)

            if data["command"] == "done":
                self.handle_done(conn, cli_addr, data)

            elif data["command"] == "need":
                self.handle_need(conn, cli_addr, data)

            elif data["command"] == "build":
                self.handle_build(conn, cli_addr, data)

            sys.stdout.flush()
        except:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            traceback.print_exc()
            the_trace = traceback.format_exc()

            logging.error("Exception from %s, Trace: %s", cli_addr,
                          the_trace)

        finally:
            # Clean up the connection
            logging.info("Finished with client %s", cli_addr)
            conn.close()

    def run(self):
        global acc_queue
        while True:
            conn, cli_addr = acc_queue.get()
            self.handle_one_client(conn, cli_addr)


class Listener (threading.Thread):

    def __init__(self):
        threading.Thread.__init__(self)
        pass

    def listen_to_connection(self):
        # Create a TCP/IP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Bind the socket to the port
        server_address = ('0.0.0.0', options.port)
        logging.info('starting up on %s port %s', server_address, options.port)
        sock.bind(server_address)

        # Listen for incoming connections
        sock.listen(128)
        return sock

    def handle_one_connection(self):
        global acc_queue

        # Wait for a connection
        conn, client_addr = self.sock.accept()
        acc_queue.put_nowait((conn, client_addr))

    def run(self):
        try:
            self.sock = self.listen_to_connection()
        except:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            the_trace = traceback.format_exc().rstrip().replace("\n", " || ")
            logging.error("Cannot listen on stocket! Traceback: %s", the_trace)
            failed = True
            return
        while True:
            self.handle_one_connection()


def shutdown(exitval=0):
    toexec = "sudo shutdown -h now"
    logging.info("SHUTTING DOWN", extra={"threadid": -1})
    if not options.noaws:
        s3_folder = get_s3_folder(options.s3_folder,
                                  options.git_rev,
                                  options.timeout_in_secs,
                                  options.mem_limit_in_mb
                                  )
        upload_log(options.s3_bucket,
                   s3_folder,
                   options.logfile_name,
                   "server-%s" % get_ip_address("eth0"))

    if not options.noshutdown:
        os.system(toexec)
        pass

    exit(exitval)

set_up_logging()
logging.info("Server called with parameters: %s",
             pprint.pformat(options, indent=4).replace("\n", " || "))

failed = False
if not options.git_rev:
    options.git_rev = get_revision()
    logging.info("Revision not given, taking HEAD: %s", options.git_rev)

server = Server()
listener = Listener()
listener.setDaemon(True)
server.setDaemon(True)

listener.start()
server.start()

if not options.noaws and False:
    #create spot instance requests
    spot_create = requester.SpotRequestor()
    spot_create.create_spots()


while threading.active_count() > 0 and not failed:
    time.sleep(0.5)
    if last_termination_sent is not None and server.ready_to_shutdown():
        diff = time.time() - last_termination_sent
        limit = 2*(options.timeout_in_secs + options.extra_time)
        if diff > limit:
            break

shutdown()
