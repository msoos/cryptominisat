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

parser.add_option("--dir", "-d"
                    , default="/home/soos/satcomp09/", dest="dir"
                    , help="Directory of files to solve"
                    )
parser.add_option("--host"
                    , default=None, dest="host"
                    , help="Host to connect to as a client"
                    )
parser.add_option("--port", "-p"
                    , default=10000, dest="port"
                    , help="Port to use", type="int"
                    )
parser.add_option("--solver"
                    , default="cryptominisat/build/cryptominisat", dest="solver"
                    , help="SAT solver to use"
                    )
parser.add_option("--basedir", "-b"
                    , default="/home/soos/", dest="basedir"
                    , help="base directory"
                    )
parser.add_option("--timeout", "-t"
                    , default=1000, dest="timeout"
                    , help="Timeout for the solver", type=int
                    )
#parse options
(options, args) = parser.parse_args()

def getN(connection, n) :
    got = 0
    fulldata = ""
    while got < n :
        data = connection.recv(n-got)
        #print >>sys.stderr, 'received "%s"' % data
        if data :
            fulldata += data
            got += len(data)
        else :
            print >>sys.stderr, 'no more data from', client_address, "ooops!"
            exit(-1)

    return fulldata


#files to solve
files_to_solve = os.listdir(options.dir)
print "Solving files from '%s', number of files: %d" % (options.dir, len(files_to_solve))
solved = {}
for f in files_to_solve :
    solved[f] = False

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = ('localhost', options.port)
print >>sys.stderr, 'starting up on %s port %s' % server_address
sock.bind(server_address)

#Listen for incoming connections
sock.listen(1)

while True:
    # Wait for a connection
    print >>sys.stderr, 'waiting for a connection...'
    connection, client_address = sock.accept()

    try:
        print >>sys.stderr, 'connection from', client_address

        data = getN(connection, 8)
        assert len(data) == 8

        if data[:4] == "done":
            #client finished with something
            length = struct.unpack('i', data[4:])[0]
            finishedwith = getN(connection, length)
            print "Client finished with ", finishedwith
            assert finishedwith in solved
            solved[finishedwith] = True

        elif data[:4] == "need" :
            #client needs something to solve

            #find one that is unsolved
            tosolve = None
            for a,b in solved.items() :
                if not b :
                    #this needs to be solved
                    tosolve = a
                    break

            #yay, everything finished!
            if tosolve == None :
                print "No more to solve sending termination"

                #indicate that we are finished, it can close
                tosend = struct.pack('i', 0)
                connection.sendall(tosend)
            else :
                #send filename that needs solving
                tosend = {}
                tosend["basedir"] = options.basedir
                tosend["solver"]  = options.solver
                tosend["timeout"] = options.timeout
                tosend["filename"] = tosolve
                data = pickle.dumps(tosend)

                print "sending that file '%s' needs to be solved" % tosend["filename"]
                tosend = struct.pack('i', len(data)) + data
                #print ":".join("{0:x}".format(ord(c)) for c in tosend)
                connection.sendall(tosend)

    finally:
        # Clean up the connection
        print "Finished with client"
        connection.close()

