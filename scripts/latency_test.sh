#!/usr/bin/env bash
set -euo pipefail

export LD_LIBRARY_PATH=$EXTRA/lib${LD_LIBRARY_PATH:+:}${LD_LIBRARY_PATH:-}

readonly scriptDir=$(dirname $0)

readonly tmpDir=/tmp/latency-test-$$
trap "rm -rf $tmpDir" EXIT
mkdir -p $tmpDir

readonly BIN=$1

function main
{
  export SIGNALS_SMD_PATH=$BIN

  g++ src/main_latency.cpp $BIN/signals-unity-bridge.so \
    -o $tmpDir/main_latency.exe

  $scriptDir/dash-live-simulator-server.sh &
  pid=$!

  sleep 1.0
  exitCode=0
  $tmpDir/main_latency.exe "http://127.0.0.1:9000/latency.mpd" || exitCode=$?

  kill $pid
  wait $pid || true

  if [ ! $exitCode = 0 ] ; then
    exit 1
  fi
}

main

