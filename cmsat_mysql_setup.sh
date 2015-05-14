#!/bin/bash

SETUP_DB="create database cmsat;
    create user 'cmsat_solver'@'localhost' identified by '';
    grant insert,update on cmsat.* to 'cmsat_solver'@'localhost';
    create user 'cmsat_presenter'@'localhost' identified by '';
    grant select on cmsat.* to 'cmsat_presenter'@'localhost';"

if [ $1 ]; then
    echo "Using password '$1' for root access to mysql"
    echo "$SETUP_DB" | mysql -u root -p "$1"
    if [ $? -ne 0 ]; then
        echo "ERROR: Cannot create database!";
        exit -1;
    fi
    mysql -u root -p "$1" cmsat < cmsat_tablestructure.sql
    if [ $? -ne 0 ]; then
        echo "ERROR: Cannot add tables to database!";
        exit -1;
    fi
else
    echo "Not using any password for root access to mysql"
    echo "$SETUP_DB" | mysql -u root
    if [ $? -ne 0 ]; then
        echo "ERROR: Cannot create database!";
        exit -1;
    fi
    mysql -u root cmsat < cmsat_tablestructure.sql
    if [ $? -ne 0 ]; then
        echo "ERROR: Cannot add tables to database!";
        exit -1;
    fi
fi

echo "Created DB, set up users cmsat_solver and cmsat_presenter, and added tables"