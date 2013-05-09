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
parser.add_option("--solver"
                    , default="cryptominisat/build/cryptominisat", dest="solver"
                    , help="SAT solver to use"
                    )
parser.add_option("--basedir", "-b"
                    , default="/home/soos", dest="basedir"
                    , help="base directory"
                    )

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

def connectClient() :
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

#as long as we are up and running
while True :
    sock = connectClient()

    #ask for stuff to solve
    print "asking for stuff to solve..."
    sock.sendall("need    ".format("ascii"))

    #get stuff to solve
    data = getN(sock, 4)
    assert len(data) == 4
    length = struct.unpack('i', data)[0]
    print "length of file to solve: ", length

    #nothing more to solve?
    if length == 0 :
        print "Client received that there is nothing more to solve, exiting"
        exit(0)

    tosolve = getN(sock, length)
    print "Have to solve ", tosolve
    sock.close()

    toexec = "%s/%s %s/cnfs/%s > %s/output/%s.out 2>&1" \
    % (options.basedir, options.solver, options.basedir, tosolve, options.basedir, tosolve)

    print "exiecuting '%s'" % toexec

    #solving 'name'
    #....
    print "solved '%s'" % tosolve

    sock = connectClient()
    tosolve = tosolve.format("ascii")
    tosend = "done".format("ascii") + struct.pack('i', len(tosolve)) + tosolve
    print "Sending that we finished", tosolve
    sock.sendall(tosend)
    print "Sent that we finished", tosolve