#!/bin/bash
set -e

CCompiler="cc"
CFlags="-O2"
LDFlags="-lzstd"
Include=""
Out="build/vcgs"
Jobs="$(nproc)"

rm -rf build
mkdir -p build/vcgsb

CompileVcgs() {
  File="$1"
  Object="build/vcgsb/$(echo "$File" | sed 's#/#_#g; s#\.c$#.o#')"

  echo "  CC  $File"
  $CCompiler $CFlags $Include -c "$File" \
    -o "$Object"
}

export -f CompileVcgs
export CCompiler CFlags Include

find src -name "*.c" |
  xargs -P "$Jobs" -n 1 bash -c 'CompileVcgs "$1"' _

echo "  LD  $Out"
$CCompiler $CFlags $LDFlags build/vcgsb/*.o -o "$Out"
