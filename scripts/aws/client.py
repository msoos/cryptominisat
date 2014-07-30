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
        return "%s/%s" % (self.tosolve["basedir"], self.tosolve["solver"])

    def execute(self) :
        toexec = "%s %s/%s/%s" % ( \
            self.solver_filename(), \
            self.tosolve["basedir"], \
            self.tosolve["cnf_files_dir"], \
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

    def copy_solution_to_server(self) :
        toexec = "scp %s %s:%s/%s/" % ( \
            self.get_output_filename(), \
            options.host, \
            self.tosolve["basedir"], \
            self.tosolve["solutionto"] \
        )
        print "Executing '%s' to put the file to the right place" % toexec

        process = subprocess.Popen(toexec.rsplit(), stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
        consoleOutput, err = process.communicate()
        print "SCP stdout:", consoleOutput, " stderr:", err
        if process.returncode != 0:
            print "Error copying file\n"
            exit(-1)

        #remove temporary solution file
        os.unlink("/tmp/%s-%d.out" % (self.tosolve["filename"], self.tosolve["unique_counter"],))
        return err == ""

    def run(self):
        print "Starting " + self.name

        #as long as there is something to do
        while True :
            sock = self.connect_client()

            #ask for stuff to solve
            print "asking for stuff to solve..."
            sock.sendall("need    ".format("ascii"))

            #get stuff to solve
            data = self.get_n_bytes_from_connection(sock, 4)
            assert len(data) == 4
            length = struct.unpack('i', data)[0]
            print "length of tosolve data: ", length

            #nothing more to solve?
            if length == 0 :
                print "Client received that there is nothing more to solve, exiting"
                return

            data = self.get_n_bytes_from_connection(sock, length)
            self.tosolve = pickle.loads(data)
            print "Have to solve ", pp.pprint(self.tosolve)
            sock.close()

            self.execute()
            err = self.copy_solution_to_server()
            if err:
                print "There was an error with copy:", err

            sock = self.connect_client()
            tosend = "done" + struct.pack('i', len(self.tosolve["filename"])) + self.tosolve["filename"]
            sock.sendall(tosend)
            print "Sent that we finished", self.tosolve["filename"]

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
