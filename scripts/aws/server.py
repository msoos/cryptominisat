#!/usr/bin/env python
# -*- coding: utf-8 -*-

# request micro spot instance:
# ec2-request-spot-instance ami-3e6ffb0e -p 0.030 -g default -t t1.micro --user-data "myip"

import os
import ssl
import socket
import sys
import optparse
import struct
import pickle
import time
from collections import namedtuple

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

parser.add_option("--cnfdir", "-d"
                    , default="examples/satcomp091113", dest="cnf_files_dir"
                    , help="Directory of files to solve"
                    )
parser.add_option("--port", "-p"
                    , default=10000, dest="port"
                    , help="Port to use", type="int"
                    )
parser.add_option("--solver", "-s"
                    , default="cryptominisat/build/cryptominisat", dest="solver"
                    , help="SAT solver to use"
                    )
parser.add_option("--basedir", "-b"
                    , default="/home/soos/media/sat", dest="basedir"
                    , help="base directory"
                    )
parser.add_option("--memory", "-m"
                    , default=1024, dest="memory"
                    , help="Memory in MB for the process", type=int
                    )

parser.add_option("--solto"
                    , default="output", dest="solutionto"
                    , help="Put finished data here"
                    )
parser.add_option("--tout", "-t"
                    , default=1000, dest="timeout"
                    , help="Timeout for the solver in seconds", type=int
                    )
parser.add_option("--etout"
                    , default=50, dest="extratimeout"
                    , help="Extra time to wait before slave reports that the problem has been solved"
                    )
#parse options
(options, args) = parser.parse_args()

def enum(**enums):
    return type('Enum', (), enums)
State = enum(unsent=1, sent=2, finished=3)
SolveState = namedtuple('SolveSate', ['state', 'time'])

class Server :
    def __init__(self) :

        #files to solve
        files_location = "%s/%s" % (options.basedir, options.cnf_files_dir)
        files_to_solve = os.listdir(files_location)
        #print files_to_solve
        print "Solving files from '%s', number of files: %d" % (files_location, len(files_to_solve))

        self.solved = {}
        for f in files_to_solve :
            self.solved[f] = SolveState(State.unsent, 0.0)

        self.unique_counter = 0

    def listen_to_connection(self) :
        # Create a TCP/IP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Bind the socket to the port
        server_address = ('localhost', options.port)
        print >>sys.stderr, 'starting up on %s port %s' % server_address
        sock.bind(server_address)

        #Listen for incoming connections
        sock.listen(1)

        return sock

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
                #print >>sys.stderr, 'no more data from', client_address, "ooops!"
                print >>sys.stderr, 'no more data from client ooops!'
                exit(-1)

        return fulldata

    def handle_done(self, connection, data) :
        #client finished with something
        length = struct.unpack('i', data[4:])[0]
        finishedwith = self.get_n_bytes_from_connection(connection, length)
        print "Client finished with ", finishedwith
        assert finishedwith in self.solved
        self.solved[finishedwith] = SolveState(State.finished, 0.0)

    def find_something_to_solve(self) :
        tosolve = None
        for a,b in self.solved.items() :
            if b.state == State.unsent :
                tosolve = a
                break

        #...or has timed out (slave died probably)
        if tosolve == None:
            for a,b in self.solved.items() :
             if b.state == State.sent and b.time > time.clock() + options.timeout + options.extratimeout :
                #this needs to be solved
                tosolve = a
                break

        return tosolve

    #client needs something to solve
    def handle_need(self, connection, data) :
        tosolve = self.find_something_to_solve()

        #yay, everything finished!
        if tosolve == None :
            print "No more to solve sending termination"

            #indicate that we are finished, it can close
            tosend = struct.pack('i', 0)
            connection.sendall(tosend)
        else :
            #set timer that we have sent this to be solved
            self.solved[tosolve] = SolveState(State.sent, time.clock())

            #send data for solving
            tosend = {}
            tosend["basedir"] = options.basedir
            tosend["solver"]  = options.solver
            tosend["timeout"] = options.timeout
            tosend["memory"] = options.memory*1024*1024 #bytes from MB
            tosend["solutionto"] = options.solutionto
            tosend["cnf_files_dir"] = options.cnf_files_dir
            tosend["filename"] = tosolve
            tosend["unique_counter"] = self.unique_counter
            self.unique_counter+=1
            data = pickle.dumps(tosend)

            print "sending that file '%s' needs to be solved" % tosend["filename"]
            tosend = struct.pack('i', len(data)) + data
            connection.sendall(tosend)

    def handle_one_connection(self):
        # Wait for a connection
        print >>sys.stderr, 'waiting for a connection...'
        connection, client_address = self.sock.accept()

        try:
            print >>sys.stderr, 'connection from', client_address

            #8: 4B for 'need'/'done' and 4B for integer of following struct size in case of "done'
            data = self.get_n_bytes_from_connection(connection, 8)
            assert len(data) == 8

            if data[:4] == "done" :
                self.handle_done(connection, data)

            elif data[:4] == "need" :
               self.handle_need(connection, data)

        finally:
            # Clean up the connection
            print "Finished with client"
            connection.close()

    def handle_all_connections(self):
        self.sock = self.listen_to_connection()
        while True:
            self.handle_one_connection()


server = Server()
server.handle_all_connections()