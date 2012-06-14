#!/bin/bash
#Example: do_sd.sh 51

MON=$1
SDDIR=${HOME}/sleepData/SD
INFILE=$SDDIR/Monitor$MON.txt
LOGFILE=$SDDIR/sd_log.txt
SD=${HOME}/git/ardufly/DAMrealTime/sleepDeprivator.py

echo 'Performing SD on monitor' $MON
python $SD -d $INFILE >> $LOGFILE

