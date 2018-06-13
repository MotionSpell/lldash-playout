#!/usr/bin/env bash
set -euo pipefail

readonly tmpDir=/tmp/signals-test-$$
trap "rm -rf $tmpDir" EXIT
mkdir -p $tmpDir

EXTRA=${EXTRA-$PWD/sysroot}

# required for MSYS
export PATH=$PATH:$EXTRA/bin:/mingw64/bin

# required for GNU/Linux
export LD_LIBRARY_PATH=$EXTRA/lib${LD_LIBRARY_PATH:+:}${LD_LIBRARY_PATH:-}

# required for ?
export DYLD_LIBRARY_PATH=$EXTRA/lib${DYLD_LIBRARY_PATH:+:}${DYLD_LIBRARY_PATH:-}

function main
{
  run_test load_library
  echo "OK"
}

function run_test
{
  local name="$1"
  echo "* $name"
  "$name"
}

function load_library
{
  $BIN/loader.exe $BIN/signals-unity-bridge.so signals/data/sine.mp3
}

main "$@"

