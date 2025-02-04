#!/usr/bin/env bash
set -euo pipefail

readonly scriptDir=$(dirname $0)

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
  run_test check_exports
  run_test load_library
  run_test api_tests
  echo "OK"
}

function run_test
{
  local name="$1"
  echo "* $name"
  "$name"
}

function get_exports
{
  nm -D $1 | grep " T " | sed 's/.* T //' | sort -f
}

function check_exports
{
  case $(nm --version | head -n 1) in
    *GNU*)
      ;;
    *)
      echo "non-GNU nm detected: skipping export checking" >&2
      return 0
  esac

  get_exports "$BIN/signals-unity-bridge.so" > $tmpDir/exports.new
  diff -Naur $scriptDir/exports.ref $tmpDir/exports.new
}

function load_library
{
  $BIN/loader.exe $BIN/signals-unity-bridge.so "data/test.mp4"
}

function api_tests
{
  export SIGNALS_SMD_PATH=$BIN
  g++ -std=c++11 $scriptDir/tests.cpp -pthread -I$scriptDir/../src -Ibin -o $tmpDir/tests.exe $BIN/signals-unity-bridge.so
  $tmpDir/tests.exe
}

main "$@"

