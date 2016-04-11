#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import os
import socket
import sys
import struct
import pickle
import time
import pprint
import traceback
import subprocess
import Queue
import threading
import logging
import server_option_parser

# for importing in systems where "." is not in the PATH
import glob
sys.path.append(os.getcwd())
from common_aws import *
import RequestSpotClient


def get_revision():
    _, solvername = os.path.split(options.base_dir + options.solver)
    if solvername != "cryptominisat4":
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


def send_command(sock, command, tosend={}):
    tosend["command"] = command
    tosend = pickle.dumps(tosend)
    tosend = struct.pack('!q', len(tosend)) + tosend
    sock.sendall(tosend)


class ToSolve:

    def __init__(self, num, name):
        self.num = num
        self.name = name

    def __str__(self):
        return "%s (num: %d)" % (self.name, self.num)


class Server (threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.files_available = []
        self.files_finished = []
        self.files = {}

        os.system("aws s3 cp s3://msoos-solve-data/solvers/%s . --region us-west-2" % options.cnf_dir)
        fnames = open(options.cnf_dir, "r")
        logging.info("CNF dir is %s", options.cnf_dir)
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

        logging.info("Finished with file %s (num %d), got files %s",
                     self.files[indata["file_num"]], indata["file_num"],
                     indata["files"])
        self.files_finished.append(indata["file_num"])
        if file_num in self.files_running:
            del self.files_running[file_num]

        logging.info("Num files_available: %d Num files_finished %d",
                     len(self.files_available), len(self.files_finished))

        self.rename_files_to_final(indata["files"])
        sys.stdout.flush()

    def rename_files_to_final(self, files):
        for fnames in files:
            logging.info("Renaming file %s to %s",
                         fnames[0], fnames[1])
            ret = os.system("aws s3 mv s3://%s/%s s3://%s/%s --region us-west-2" %
                            (options.s3_bucket, fnames[0], options.s3_bucket,
                             fnames[1]))

    def check_for_dead_files(self):
        this_time = time.time()
        files_to_remove_from_files_running = []
        for file_num, starttime in self.files_running.iteritems():
            duration = this_time - starttime
            # print("* death check. running:" , file_num, " duration: ",
            # duration)
            if duration > options.timeout_in_secs*options.tout_mult:
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
        tosend = self.default_tosend()
        logging.info("Sending git revision %s to %s", options.git_rev,
                     cli_addr)
        send_command(connection, "build_data", tosend)

    def send_termination(self, connection, cli_addr):
        tosend = {}
        tosend["noshutdown"] = options.noshutdown
        send_command(connection, "finish", tosend)

        logging.info("No more to solve, terminating %s", cli_addr)
        global last_termination_sent
        last_termination_sent = time.time()

    def send_wait(self, connection, cli_addr):
        tosend = {}
        tosend["noshutdown"] = options.noshutdown
        logging.info("Everything is in sent queue, sending wait to %s", cli_addr)
        send_command(connection, "wait", tosend)

    def default_tosend(self):
        tosend = {}
        tosend["solver"] = options.solver
        tosend["git_rev"] = options.git_rev
        tosend["stats"] = options.stats
        tosend["s3_bucket"] = options.s3_bucket
        tosend["s3_folder"] = options.s3_folder
        tosend["timeout_in_secs"] = options.timeout_in_secs
        tosend["mem_limit_in_mb"] = options.mem_limit_in_mb
        tosend["noshutdown"] = options.noshutdown
        tosend["cnf_dir"] = options.cnf_dir
        tosend["extra_opts"] = options.extra_opts
        tosend["drat"] = options.drat

        return tosend

    def send_one_to_solve(self, connection, cli_addr, file_num):
        # set timer that we have sent this to be solved
        self.files_running[file_num] = time.time()
        filename = self.files[file_num].name

        tosend = self.default_tosend()
        tosend["file_num"] = file_num
        tosend["cnf_filename"] = filename
        tosend["uniq_cnt"] = str(self.uniq_cnt)
        logging.info("Sending file %s (num %d) to %s",
                     filename, file_num, cli_addr)
        send_command(connection, "solve", tosend)
        self.uniq_cnt += 1

    def handle_need(self, connection, cli_addr, indata):
        # TODO don't ignore 'indata' for solving CNF instances, use it to
        # opitimize for uptime
        file_num = self.find_something_to_solve()

        if file_num is None:
            if len(self.files_running) == 0:
                self.send_termination(connection, cli_addr)
            else:
                self.send_wait(connection, cli_addr)
        else:
            self.send_one_to_solve(connection, cli_addr, file_num)

    def handle_one_client(self, conn, cli_addr):
        try:
            logging.info("connection from %s", cli_addr)

            data = get_n_bytes_from_connection(conn, 8)
            length = struct.unpack('!q', data)[0]
            data = get_n_bytes_from_connection(conn, length)
            data = pickle.loads(data)

            if data["command"] == "done":
                self.handle_done(conn, cli_addr, data)

            if data["command"] == "error":
                shutdown(-1)
                raise

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
            shutdown(-1)
            raise
        while True:
            self.handle_one_connection()


class SpotManager (threading.Thread):

    def __init__(self, noshutdown):
        threading.Thread.__init__(self)
        self.spot_creator = RequestSpotClient.RequestSpotClient(
            options.cnf_dir == "test", noshutdown=noshutdown,
            count=options.client_count)

    def run(self):
        while True:
            try:
                if not server.ready_to_shutdown():
                    self.spot_creator.create_spots_if_needed()
            except:
                exc_type, exc_value, exc_traceback = sys.exc_info()
                the_trace = traceback.format_exc().rstrip().replace("\n", " || ")
                logging.error("Cannot create spots! Traceback: %s", the_trace)

            time.sleep(60)


def shutdown(exitval=0):
    toexec = "sudo shutdown -h now"
    logging.info("SHUTTING DOWN")

    #send email
    try:
        email_subject = "Server shutting down "
        if exitval == 0:
            email_subject += "OK"
        else:
            email_subject += "FAIL"

        full_s3_folder = get_s3_folder(options.s3_folder,
                                       options.git_rev,
                                       options.timeout_in_secs,
                                       options.mem_limit_in_mb)
        text = """Server finished. Please download the final data:

mkdir {0}
cd {0}
aws s3 cp --recursive s3://{1}/{0}/ .

Don't forget to:

* check volume
* check EC2 still running

So long and thanks for all the fish!
""".format(full_s3_folder, options.s3_bucket)
        send_email(email_subject, text, options.logfile_name)
    except:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        the_trace = traceback.format_exc().rstrip().replace("\n", " || ")
        logging.error("Cannot send email! Traceback: %s", the_trace)

    #upload log
    upload_log(options.s3_bucket,
               full_s3_folder,
               options.logfile_name,
               "server-%s" % get_ip_address("eth0"))

    if not options.noshutdown:
        os.system(toexec)
        pass

    exit(exitval)


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

if __name__ == "__main__":
    global options
    global args
    options, args = server_option_parser.parse_arguments()
    if options.drat:
        assert "cryptominisat" in options.solver

    global acc_queue
    acc_queue = Queue.Queue()
    last_termination_sent = None

    set_up_logging()
    logging.info("Server called with parameters: %s",
                 pprint.pformat(options, indent=4).replace("\n", " || "))

    if not options.git_rev:
        options.git_rev = get_revision()
        logging.info("Revision not given, taking HEAD: %s", options.git_rev)

    server = Server()
    listener = Listener()
    spotmanager = SpotManager(options.noshutdown)
    listener.setDaemon(True)
    server.setDaemon(True)
    spotmanager.setDaemon(True)

    listener.start()
    server.start()
    time.sleep(20)
    spotmanager.start()

    while threading.active_count() > 0:
        time.sleep(0.5)
        if last_termination_sent is not None and server.ready_to_shutdown():
            diff = time.time() - last_termination_sent
            limit = 100
            if diff > limit:
                break

    shutdown()
