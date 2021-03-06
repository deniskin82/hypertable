#!/usr/bin/env bash

HT_HOME=${INSTALL_DIR:-"$HOME/hypertable/current"}
HYPERTABLE_HOME=$HT_HOME
PIDFILE=$HT_HOME/run/Hypertable.Master.pid
LAUNCHER_PIDFILE=$HT_HOME/run/Hypertable.MasterLauncher.pid
DUMP_METALOG=$HT_HOME/bin/metalog_dump
METALOG="/hypertable/servers/master/log/mml"
SCRIPT_DIR=`dirname $0`

. $HT_HOME/bin/ht-env.sh

# Kill launcher if running & store pid of this launcher
if [ -f $LAUNCHER_PIDFILE ]; then
  kill -9 `cat $LAUNCHER_PIDFILE`
  rm -f $LAUNCHER_PIDFILE
fi
echo "$$" > $LAUNCHER_PIDFILE

# Kill Master if running
if [ -f $PIDFILE ]; then
  kill -9 `cat $PIDFILE`
  rm -f $PIDFILE
fi

# Dumping cores slows things down unnecessarily for normal test runs
#ulimit -c 0

$HT_HOME/bin/Hypertable.Master --verbose --pidfile=$PIDFILE \
    --config=${SCRIPT_DIR}/test.cfg $1

# Exit if base run
if [ -z $1 ]; then
    \rm -f $LAUNCHER_PIDFILE
    exit
fi

echo ""
echo "!!!! CRASH ($@) !!!!"
echo ""

echo "MML entries:"
$DUMP_METALOG $METALOG

# Kill RangeServer if requested
if [ -f $2 ]; then
  kill -9 `cat $2`
  \rm -f $2
fi


#$HT_HOME/bin/ht valgrind -v --log-file=vg_rs1.log --track-origins=yes \
$HT_HOME/bin/Hypertable.Master --pidfile=$PIDFILE --verbose \
    --config=${SCRIPT_DIR}/test.cfg 

\rm -f $LAUNCHER_PIDFILE