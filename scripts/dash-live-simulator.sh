#!/usr/bin/env bash
# Under Debian: apt install ucspi-tcp
set -eou pipefail
serverPid=""

readonly tmpDir=/tmp/http-fake-server-$$
trap 'rm -rf $tmpDir ; test -z "$serverPid" || kill $serverPid' EXIT
mkdir -p "$tmpDir"

readonly scriptDir=$(dirname $0)
g++ $scriptDir/dash-live-simulator-cgi.cpp -o $tmpDir/fake-server-cgi.exe
tcpserver -D 127.0.0.1 9000 $tmpDir/fake-server-cgi.exe &
serverPid=$!
wait $serverPid

