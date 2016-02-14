#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import os
import socket
import sys
import optparse
import struct
import pickle
import threading
import random
import time
import subprocess
import resource
import pprint
import traceback
import boto
import socket
import fcntl
import struct
import logging

# for importing in systems where "." is not in the PATH
import glob
sys.path.append(os.getcwd())
from common_aws import *

pp = pprint.PrettyPrinter(depth=6)


class PlainHelpFormatter(optparse.IndentedHelpFormatter):

    def format_description(self, description):
        if description:
            return description + "\n"
        else:
            return ""


def uptime():
    with open('/proc/uptime', 'r') as f:
        return float(f.readline().split()[0])

    return None


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


def connect_client(threadID):
    # Create a socket object
    sock = socket.socket()

    # Get local machine name
    if options.host is None:
        print("You must supply the host to connect to as a client")
        exit(-1)

    logging.info("Getting host by name %s", options.host,
                 extra={"threadid": threadID})
    host = socket.gethostbyname_ex(options.host)
    logging.info("Connecting to host %s", host,
                 extra={"threadid": threadID})
    sock.connect((host[2][0], options.port))

    return sock


def ask_for_data_to_solve(sock, command, threadID):
    logging.info("Asking for %s", command,
                 extra={"threadid": threadID})
    tosend = {}
    tosend["uptime"] = uptime()
    tosend["command"] = command
    tosend = pickle.dumps(tosend)
    tosend = struct.pack('!q', len(tosend)) + tosend
    sock.sendall(tosend)

    # get stuff to solve
    data = get_n_bytes_from_connection(sock, 8)
    length = struct.unpack('!q', data)[0]
    data = get_n_bytes_from_connection(sock, length)
    indata = pickle.loads(data)
    return indata


def signal_error_to_master():
    sock = connect_client(100)
    tosend = {}
    tosend["command"] = "error"
    tosend = pickle.dumps(tosend)
    tosend = struct.pack('!q', len(tosend)) + tosend
    sock.sendall(tosend)
    sock.close()


class solverThread (threading.Thread):

    def __init__(self, threadID):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.logextra = {'threadid': self.threadID}
        self.temp_space = self.create_temp_space()
        logging.info("Initializing thread", extra=self.logextra)

    def create_temp_space(self):
        orig = options.temp_space
        newdir = orig + "/thread-%s" % self.threadID
        try:
            os.mkdir(newdir)
        except:
            logging.info("Directory %s already exists.", newdir,
                         extra=self.logextra)

        return newdir

    def setlimits(self):
        # sys.stdout.write("Setting resource limit in child (pid %d). Time %d s
        # Mem %d MB\n" % (os.getpid(), self.indata["timeout_in_secs"],
        # self.indata["mem_limit_in_mb"]))
        resource.setrlimit(resource.RLIMIT_CPU, (
            self.indata["timeout_in_secs"], self.indata["timeout_in_secs"]))
        resource.setrlimit(resource.RLIMIT_DATA, (
            self.indata["mem_limit_in_mb"] * 1024 * 1024,
            self.indata["mem_limit_in_mb"] * 1024 * 1024))

    def get_output_fname(self):
        return "%s/%s" % (
            self.temp_space,
            self.indata["cnf_filename"]
        )

    def get_stdout_fname(self):
        return self.get_output_fname() + "-" + self.indata["uniq_cnt"] + ".stdout"

    def get_stderr_fname(self):
        return self.get_output_fname() + "-" + self.indata["uniq_cnt"] + ".stderr"

    def get_perf_fname(self):
        return self.get_output_fname() + "-" + self.indata["uniq_cnt"] + ".perf"

    def get_sqlite_fname(self):
        return self.get_output_fname() + ".sqlite"

    def get_toexec(self):
        extra_opts = ""
        if "cryptominisat" in self.indata["solver"]:
            extra_opts = " --printsol 0 --sql 2 --clid"

        extra_opts += " " + self.indata["extra_opts"] + " "

        os.system("aws s3 cp s3://msoos-solve-data/%s/%s %s/ --region us-west-2" % (
            self.indata["cnf_dir"], self.indata["cnf_filename"], self.temp_space))

        # os.system("touch %s" % self.get_perf_fname())
        # toexec = "sudo perf record -o %s %s %s %s/%s" % (self.get_perf_fname(),
        toexec = "%s %s %s/%s" % (
            self.indata["solver"],
            extra_opts,
            self.temp_space,
            self.indata["cnf_filename"])

        #add DRAT in case of cryptominisat
        if "cryptominisat" in self.indata["solver"]:
            toexec += " %s/%s" % (self.temp_space, "drat")

        return toexec

    def execute_solver(self):
        toexec = self.get_toexec()
        stdout_file = open(self.get_stdout_fname(), "w+")
        stderr_file = open(self.get_stderr_fname(), "w+")

        # limit time
        limits_printed = "Thread %d executing '%s' with timeout %d s  and memout %d MB" % (
            self.threadID,
            toexec,
            self.indata["timeout_in_secs"],
            self.indata["mem_limit_in_mb"]
        )
        logging.info(limits_printed, extra=self.logextra)
        stderr_file.write(limits_printed + "\n")
        stderr_file.flush()
        stdout_file.write(limits_printed + "\n")
        stdout_file.flush()

        tstart = time.time()
        p = subprocess.Popen(
            toexec.rsplit(), stderr=stderr_file, stdout=stdout_file,
            preexec_fn=self.setlimits)
        p.wait()
        tend = time.time()

        towrite = "Finished in %f seconds by thread %s return code: %d\n" % (
            tend - tstart, self.threadID, p.returncode)
        stderr_file.write(towrite)
        stdout_file.write(towrite)
        stderr_file.close()
        stdout_file.close()
        logging.info(towrite.strip(), extra=self.logextra)
        os.unlink("%s/%s" % (self.temp_space, self.indata["cnf_filename"]))

        return p.returncode, toexec

    def run_drat_trim(self):
        toexec = "drat-trim/drat-trim2 %s/%s %s/%s -l %s/lemmas" % (
            self.temp_space,
            self.indata["cnf_filename"],
            self.temp_space,
            "drat",
            self.temp_space)

        stdout_file = open(self.get_stdout_fname(), "w+")
        stderr_file = open(self.get_stderr_fname(), "w+")
        tstart = time.time()
        p = subprocess.Popen(
            toexec.rsplit(), stderr=stderr_file, stdout=stdout_file,
            preexec_fn=self.setlimits)
        p.wait()
        tend = time.time()

        towrite = "Finished DRAT-TRIM2 in %f seconds by thread %s return code: %d\n" % (
            tend - tstart, self.threadID, p.returncode)
        stderr_file.write(towrite)
        stdout_file.write(towrite)
        stderr_file.close()
        stdout_file.close()

        os.unlink("%s/%s" % (self.temp_space, "drat"))

        return p.returncode

    def parse_lemmas(self):
        logging.info("Updating sqlite with DRAT info", extra=self.logextra)
        #TODO

        os.unlink("%s/%s" % (self.temp_space, "lemmas"))

    def create_url(self, bucket, folder, key):
        return 'https://%s.s3.amazonaws.com/%s/%s' % (bucket, folder, key)

    def copy_solution_to_s3(self):
        exists = boto_conn.lookup(self.indata["s3_bucket"])
        if not exists:
            boto_conn.create_bucket(self.indata["s3_bucket"])
        boto_bucket = boto_conn.get_bucket(self.indata["s3_bucket"])
        k = boto.s3.key.Key(boto_bucket)

        s3_folder = get_s3_folder(self.indata["s3_folder"],
                                  self.indata["git_rev"],
                                  self.indata["timeout_in_secs"],
                                  self.indata["mem_limit_in_mb"])

        s3_folder_and_fname = s3_folder + "/" + self.indata[
            "cnf_filename"] + "-" + self.indata["uniq_cnt"]
        s3_folder_and_fname_clean = s3_folder + "/" + self.indata[
            "cnf_filename"]

        toreturn = []

        # stdout
        os.system("gzip -f %s" % self.get_stdout_fname())
        fname = s3_folder_and_fname + ".stdout.gz-tmp"
        fname_clean = s3_folder_and_fname_clean + ".stdout.gz"
        k.key = fname
        boto_bucket.delete_key(k)
        k.set_contents_from_filename(self.get_stdout_fname() + ".gz")
        toreturn.append([fname, fname_clean])

        # stderr
        os.system("gzip -f %s" % self.get_stderr_fname())
        fname = s3_folder_and_fname + ".stderr.gz-tmp"
        fname_clean = s3_folder_and_fname_clean + ".stderr.gz"
        k.key = fname
        boto_bucket.delete_key(k)
        k.set_contents_from_filename(self.get_stderr_fname() + ".gz")
        toreturn.append([fname, fname_clean])

        # perf
        # os.system("gzip -f %s" % self.get_perf_fname())
        # k.key = s3_folder_and_fname + ".perf.gz"
        # boto_bucket.delete_key(k)
        # k.set_contents_from_filename(self.get_perf_fname() + ".gz")

        # sqlite
        if "cryptominisat" in self.indata["solver"]:
            if self.run_drat_trim() != 0:
                self.parse_lemmas()

            os.system("gzip -f %s" % self.get_sqlite_fname())
            fname = s3_folder_and_fname + ".sqlite.gz-tmp"
            fname_clean = s3_folder_and_fname_clean + ".sqlite.gz"
            k.key = fname
            boto_bucket.delete_key(k)
            k.set_contents_from_filename(self.get_sqlite_fname() + ".gz")
            toreturn.append([fname, fname_clean])

        logging.info("Uploaded stdout+stderr+sqlite+perf files: %s",
                     toreturn, extra=self.logextra)

        os.unlink(self.get_stdout_fname() + ".gz")
        os.unlink(self.get_stderr_fname() + ".gz")
        os.unlink(self.get_sqlite_fname() + ".gz")
        # os.unlink(self.get_perf_fname() + ".gz")

        return toreturn

    def run_loop(self):
        global exitapp
        num_connect_problems = 0
        while not exitapp:
            if (num_connect_problems >= 20):
                logging.error("Too many connection problems, exiting.",
                              extra=self.logextra)
                exitapp = True
                return

            time.sleep(random.randint(0, 100) / 20.0)
            try:
                sock = connect_client(self.threadID)
            except:
                exc_type, exc_value, exc_traceback = sys.exc_info()
                the_trace = traceback.format_exc().rstrip().replace("\n", " || ")
                logging.warn("Problem trying to connect"
                             "waiting and re-connecting."
                             " Trace: %s", the_trace,
                             extra=self.logextra)
                time.sleep(3)
                num_connect_problems += 1
                continue

            self.indata = ask_for_data_to_solve(sock, "need", self.threadID)
            sock.close()

            logging.info("Got data from server %s",
                         pprint.pformat(self.indata, indent=4).replace("\n", " || "),
                         extra=self.logextra)
            options.noshutdown |= self.indata["noshutdown"]

            # handle 'finish'
            if self.indata["command"] == "finish":
                logging.warn("Client received that there is nothing more"
                             " to solve, exiting this thread",
                             extra=self.logextra)
                return

            # handle 'wait'
            if self.indata["command"] == "wait":
                time.sleep(20)
                continue

            # handle 'solve'
            if self.indata["command"] == "solve":
                returncode, executed = self.execute_solver()
                files = self.copy_solution_to_s3()
                self.send_back_that_we_solved(returncode, files)
                continue

            logging.error("Data unrecognised by client: %s, exiting",
                          self.logextra)
            return

        logging.info("Exit asked for by another thread. Exiting",
                     extra=self.logextra)

    def send_back_that_we_solved(self, returncode, files):
        logging.info("Trying to send to server that we are done",
                     extra=self.logextra)
        fail_connect = 0
        while True:
            if fail_connect > 5:
                logging.error("Too many errors connecting to server to"
                              " send results. Shutting down",
                              extra=self.logextra)
                shutdown(-1)
            try:
                sock = connect_client(self.threadID)
                break
            except:
                exc_type, exc_value, exc_traceback = sys.exc_info()
                the_trace = traceback.format_exc().rstrip().replace("\n", " || ")

                logging.warn("Problem, waiting and re-connecting."
                             " Trace: %s", the_trace,
                             extra=self.logextra)
                time.sleep(random.randint(0, 5) / 10.0)
                fail_connect += 1

        tosend = {}
        tosend["command"] = "done"
        tosend["file_num"] = self.indata["file_num"]
        tosend["returncode"] = returncode
        tosend["files"] = files

        tosend = pickle.dumps(tosend)
        tosend = struct.pack('!q', len(tosend)) + tosend
        sock.sendall(tosend)
        towrite = "Sent that we finished %s with retcode %d" % (
            self.indata["file_num"], returncode)

        sock.close()

    def run(self):
        logging.info("Starting thread", extra=self.logextra)
        global exitapp

        try:
            self.run_loop()
        except KeyboardInterrupt:
            exitapp = True
            raise
        except:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            the_trace = traceback.format_exc().rstrip().replace("\n", " || ")

            exitapp = True
            logging.error("Unexpected error in thread: %s", the_trace,
                          extra=self.logextra)
            shutdown(-1)
            raise


def build_cryptominisat(indata):
    ret = os.system('%s/cryptominisat/scripts/aws/build_cryptominisat.sh %s %s >> %s/build.log 2>&1' %
                    (options.base_dir, indata["git_rev"],
                     options.num_threads, options.base_dir))
    global s3_folder
    s3_folder = get_s3_folder(indata["s3_folder"],
                              indata["git_rev"],
                              indata["timeout_in_secs"],
                              indata["mem_limit_in_mb"]
                              )
    global s3_bucket
    s3_bucket = indata["s3_bucket"]
    upload_log(s3_bucket,
               s3_folder,
               "%s/build.log" % options.base_dir,
               "cli-build-%s.txt" % get_ip_address("eth0"))
    if ret != 0:
        logging.error("Error building cryptominisat, shutting down!",
                      extra={"threadid": -1}
                      )
        shutdown(-1)


def build_system():
    built_system = False
    logging.info("Building system", extra={"threadid": -1})
    while not built_system:
        try:
            sock = connect_client(-1)
        except Exception:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            the_trace = traceback.format_exc().rstrip().replace("\n", " || ")
            logging.warning("Problem, waiting and re-connecting. Error: %s",
                            the_trace,
                            extra={"threadid": -1})
            time.sleep(3)
            continue

        indata = ask_for_data_to_solve(sock, "build", -1)
        options.noshutdown |= indata["noshutdown"]
        sock.close()

        if "cryptominisat" in indata["solver"]:
            build_cryptominisat(indata)

        built_system = True


def num_cpus():
    num_cpu = 0
    cpuinfo = open("/proc/cpuinfo", "r")
    for line in cpuinfo:
        if "processor" in line:
            num_cpu += 1

    cpuinfo.close()
    return num_cpu


def shutdown(exitval=0):
    toexec = "sudo shutdown -h now"
    logging.info("SHUTTING DOWN", extra={"threadid": -1})

    #signal error to master
    if exitval != 0:
        try:
            signal_error_to_master()
        except:
            pass

    #send email
    if exitval == 0: reason = "OK"
    else: reason ="FAIL"
    try:
        send_email("Client shutting down %s" % reason,
                   "Client finished.", options.logfile_name)
    except:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        the_trace = traceback.format_exc().rstrip().replace("\n", " || ")
        logging.error("Cannot send email! Traceback: %s", the_trace,
                      extra={"threadid": -1})

    #upload log
    global s3_bucket
    global s3_folder
    upload_log(s3_bucket,
               s3_folder,
               options.logfile_name,
               "cli-%s.txt" % get_ip_address("eth0"))

    if not options.noshutdown:
        os.system(toexec)

    exit(exitval)


def set_up_logging():
    form = '[ %(asctime)-15s thread %(threadid)s '
    form += get_ip_address(options.network_device)
    form += " %(levelname)s  %(message)s ]"

    logformatter = logging.Formatter(form)

    consoleHandler = logging.StreamHandler()
    consoleHandler.setFormatter(logformatter)
    logging.getLogger().addHandler(consoleHandler)

    try:
        os.unlink(options.logfile_name)
    except:
        pass
    fileHandler = logging.FileHandler(options.logfile_name)
    fileHandler.setFormatter(logformatter)
    logging.getLogger().addHandler(fileHandler)

    logging.getLogger().setLevel(logging.INFO)


def start_threads():
    if options.num_threads is None:
        options.num_threads = num_cpus()/2
        options.num_threads = max(options.num_threads, 1)

    logging.info("Running with %d threads", options.num_threads,
                 extra={"threadid": -1})

    try:
        build_system()
    except:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        the_trace = traceback.format_exc().rstrip().replace("\n", " || ")
        logging.error("Error getting data for building system: %s",
                      the_trace, extra={"threadid": -1})
        shutdown(-1)

    threads = []
    options.num_threads = max(options.num_threads, 2) # for test
    for i in range(options.num_threads):
        threads.append(solverThread(i))

    for t in threads:
        t.setDaemon(True)
        t.start()


def print_to_log_local_setup():
    data = boto.utils.get_instance_metadata()
    for a, b in data:
        logging.info("%s -- %s" % (a, b))


class VolumeAdder():
    def __init__(self):
        self.conn = boto.ec2.connect_to_region(self._get_region())

    def _get_instance_id(self):
        instance_id = boto.utils.get_instance_metadata()['instance-id']
        return instance_id

    def _get_availability_zone(self):
        dat = boto.utils.get_instance_metadata()['instance-id']
        return dat["placement"]["availability-zone"]

    def _get_region(self):
        region = boto.utils.get_instance_metadata()['local-hostname'].split('.')[1]
        return region

    def add_volume(self):
        self.vol = self.conn.create_volume(50, self._get_availability_zone())
        while self.vol.status != 'available':
            print('Vol state: ', self.vol.status)
            time.sleep(10)
            self.vol.update()

        curr_vol = self.conn.get_all_volumes([self.vol.id])[0]
        assert curr_vol.status == "avalable"
        self.conn.attach_volume(self.vol.id, self._get_instance_id(), "xvdb")
        os.system("sudo mkfs.ext3 /dev/xvdb")
        os.system("mount /dev/xvdb /mnt")

    def delete_volume(self):
        self.conn.delete_volume(self.vol.id)


def parse_command_line():
    usage = "usage: %prog"
    parser = optparse.OptionParser(usage=usage, formatter=PlainHelpFormatter())
    parser.add_option("--verbose", "-v", action="store_true", default=False,
                      dest="verbose", help="Be more verbose"
                      )

    parser.add_option("--host", dest="host",
                      help="Host to connect to as a client"
                      " [default: IP of eth0]",
                      )
    parser.add_option("--port", "-p", default=10000, dest="port",
                      type="int", help="Port to use"
                      " [default: %default]",
                      )

    parser.add_option("--temp", default="/mnt/", dest="temp_space", type=str,
                      help="Temporary space to use"
                      " [default: %default]",
                      )

    parser.add_option("--noshutdown", "-n", default=False, dest="noshutdown",
                      action="store_true", help="Do not shut down"
                      )

    parser.add_option("--dir", default="/home/ubuntu/", dest="base_dir", type=str,
                      help="The home dir of cryptominisat"
                      " [default: %default]",
                      )
    parser.add_option("--net", default="eth0", dest="network_device", type=str,
                      help="The network device we will be using"
                      " [default: %default]",
                      )

    parser.add_option("--threads", dest="num_threads", type=int,
                      help="Force using this many threads")

    parser.add_option("--logfile", dest="logfile_name", type=str,
                      default="python_log.txt", help="Name of LOG file")

    (options, args) = parser.parse_args()

    return options, args


if __name__ == "__main__":
    global s3_bucket
    global s3_folder
    s3_bucket = "msoos-no-bucket"
    s3_folder = "no_s3_folder"
    options, args = parse_command_line()

    exitapp = False
    options.logfile_name = options.base_dir + options.logfile_name

    #get host
    if options.host is None:
        for line in boto.utils.get_instance_userdata().split("\n"):
            if "DATA" in line:
                options.host = line.split("=")[1].strip().strip('"')

        print("HOST has beeen set to %s" % options.host)

    try:
        set_up_logging()
        logging.info("Client called with parameters: %s",
                     pprint.pformat(options, indent=4).replace("\n", " || "),
                     extra={"threadid": -1})
        print_to_log_local_setup()
        v = VolumeAdder()
        v.add_volume()

        boto_conn = boto.connect_s3()

        #run all threads
        start_threads()
        while threading.active_count() > 1:
            time.sleep(0.1)

        #finish up
        logging.info("Exiting Main Thread, shutting down", extra={"threadid": -1})
        v.delete_volume()
    except:
        pass

    shutdown()
