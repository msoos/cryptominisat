#!/usr/bin/env python
# -*- coding: utf-8 -*-

# get user data
# GET http://169.254.169.254/latest/user-data

import os
import ssl
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
import boto
pp = pprint.PrettyPrinter(depth=6)

class PlainHelpFormatter(optparse.IndentedHelpFormatter):
    def format_description(self, description):
        if description:
            return description + "\n"
        else:
            return ""

usage = "usage: %prog"
parser = optparse.OptionParser(usage=usage, formatter=PlainHelpFormatter())
parser.add_option("--verbose", "-v", action="store_true"
                    , default=False, dest="verbose"
                    , help="Be more verbose"
                    )

parser.add_option("--host"
                    , default=None, dest="host"
                    , help="Host to connect to as a client"
                    )
parser.add_option("--port", "-p"
                    , default=10000, dest="port"
                    , help="Port to use", type="int"
                    )
parser.add_option("--threads", "-t"
                    , default=4, dest="threads"
                    , help="Number of threads to use", type="int"
                    )

(options, args) = parser.parse_args()

class solverThread (threading.Thread):
    def __init__(self, threadID, name):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.name = name

    def setlimits(self):
        #sys.stdout.write("Setting resource limit in child (pid %d): %d s\n" % (os.getpid(), maxTime))
        resource.setrlimit(resource.RLIMIT_CPU, (self.tosolve["timeout"], self.tosolve["timeout"]))
        resource.setrlimit(resource.RLIMIT_DATA, (self.tosolve["memory"], self.tosolve["memory"]))

    def get_n_bytes_from_connection(self, connection, n) :
        got = 0
        fulldata = ""
        while got < n :
            data = connection.recv(n-got)
            #print >>sys.stderr, 'received "%s"' % data
            if data :
                fulldata += data
                got += len(data)
            else :
                print >>sys.stderr, "no more data ooops!"
                exit(-1)

        return fulldata

    def connect_client(self) :
        # Create a socket object
        sock = socket.socket()

        # Get local machine name
        #host = socket.gethostname()
        if options.host == None :
            print "You must supply the host to connect to as a client"
            exit(-1)
        host = options.host
        print "Connecting to host %s ..." % host

        sock.connect((host, options.port))

        return sock

    def get_output_filename(self):
        return"/tmp/%s-%d.out" % (self.tosolve["filename"], self.tosolve["unique_counter"])

    def solver_filename(self):
        return "%s" % (self.tosolve["solver"])

    def execute(self) :
        toexec = "%s %s/%s/%s" % ( \
            self.solver_filename(), \
            self.tosolve["cnfdir"], \
            self.tosolve["filename"] \
        )
        outfile = open(self.get_output_filename(), "w")

        #limit time
        tstart = time.time()
        print "%s executing '%s' with timeout %d and memout %d" % (\
            self.name, \
            toexec, \
            self.tosolve["timeout"], \
            self.tosolve["memory"] \
        )
        p = subprocess.Popen(toexec.rsplit(), stderr=outfile, stdout=outfile, preexec_fn=self.setlimits)
        p.wait()
        outfile.close()

        time.sleep(random.randint(10,50)/1000.0)
        tend = time.time()
        print "solved '%s' in %f seconds by thread %s" % (self.tosolve["filename"], tend-tstart, self.name)
        #print "stdout:", consoleOutput, " stderr:", err

    def create_url(self, bucket, key):
        if not bucket or not key:
            return None
        return 'https://%s.s3.amazonaws.com/%s' % (bucket, key)

    def copy_solution_to_s3(self) :
        boto_bucket = boto_conn.get_bucket(self.tosolve["bucket"])
        k = boto.s3.key.Key(boto_bucket)
        k.key = self.tosolve["filename"]
        k.set_contents_from_filename(self.get_output_filename())
        print "Uploaded file"

        k.make_public()
        print "File public"


        url = self.create_url(self.tosolve["solutionto"], self.tosolve["filename"])
        print "URL: ", url, "All done."

        os.unlink(self.get_output_filename())

    def uptime(self):
        with open('/proc/uptime', 'r') as f:
            return float(f.readline().split()[0])

        return None

    def run(self):
        print "Starting " + self.name

        #as long as there is something to do
        while True :
            sock = self.connect_client()

            #ask for stuff to solve
            print "asking for stuff to solve..."
            data = {}
            data["uptime"] = self.uptime()
            data["command"] = "need"
            tosend = struct.pack('i', len(data)) + data
            sock.sendall(data)

            #get stuff to solve
            data = self.get_n_bytes_from_connection(sock, 4)
            length = struct.unpack('i', data)[0]
            data = self.get_n_bytes_from_connection(sock, length)
            self.tosolve = pickle.loads(data)

            if self.tosolve["command"] == "finish":
                print "Client received that there is nothing more to solve, exiting"
                return

            assert self.tosolve["command"] == "solve"

            print "Have to solve ", pp.pprint(self.tosolve)
            sock.close()

            self.execute()
            self.copy_solution_to_s3()

            sock = self.connect_client()
            tosend = "done" + struct.pack('i', len(self.tosolve["filename"])) + self.tosolve["filename"]
            sock.sendall(tosend)
            print "Sent that we finished", self.tosolve["filename"]

boto_conn = boto.connect_s3(boto.config.aws_access_key_id, boto.config.aws_secret_access_key)

# Create new threads
threads = []
for i in range(options.threads) :
    threads.append(solverThread(i, "Thread-%d" % i))

# Start new Threads
for t in threads:
    t.start()

# Wait for all threads to complete
for t in threads:
    t.join()

print "Exiting Main Thread"
