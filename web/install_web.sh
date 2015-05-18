#!/bin/bash
set -e
# TODO set to echo all commands, too!

# set up dygraphs
cd dygraphs
npm install
npm install gulp
gulp dist
gulp test
cd ..

# set up jquery
wget http://code.jquery.com/jquery-1.11.3.min.js

