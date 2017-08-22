#!/bin/bash
#
# Test script for starting many group viewers
#
# Sample usage :
# ./runViewers.sh --nomedia --nb=5 --url=rtmfp://localhost/test --group=G:027f02010101000103010c050e74657374011b00 --dump
#

URL="rtmfp://127.0.0.1/test"
GROUP=""
LOG=8
DUMP=""
UPDATEPERIOD=100
WINDOWDURATION=8000
NB=1
NOMEDIA=0
NOLOG=0

for i in "$@"
do
case $i in
    --url=*)
    URL="${i#*=}"
    shift # past argument=value
    ;;
    --group=*)
    GROUP="--netGroup=${i#*=}"
    shift # past argument=value
    ;;
    --log=*)
    LOG="${i#*=}"
    shift # past argument=value
    ;;
    --dump)
    DUMP="--dump"
    ;;
    --nomedia)
    NOMEDIA=1
    ;;
    --nolog)
    NOLOG=1
    ;;
    --nb=*)
    NB="${i#*=}"
    shift # past argument=value
    ;;
    --updatePeriod=*)
    UPDATEPERIOD="${i#*=}"
    shift # past argument=value
    ;;
    --windowDuration=*)
    WINDOWDURATION="${i#*=}"
    shift # past argument=value
    ;;
    *)
    echo "Unknown option $i"
    exit -1
    ;;
esac
done


for (( i=1; i<=$NB; i++ ))
do
  MEDIAFILE=$([ $NOMEDIA == 0 ] && echo "--mediaFile=out$i.flv" || echo "")
  LOGFILE=$([ $NOLOG == 0 ] && echo "--logFile=testPlay$i.log" || echo "")
  echo "./TestClient --url=$URL $GROUP --log=$LOG $DUMP --updatePeriod=$UPDATEPERIOD --windowDuration=$WINDOWDURATION $LOGFILE $MEDIAFILE &>/dev/null &"
  ./TestClient --url=$URL $GROUP --log=$LOG $DUMP --updatePeriod=$UPDATEPERIOD --windowDuration=$WINDOWDURATION $LOGFILE $MEDIAFILE &>/dev/null &
  sleep 1
done

