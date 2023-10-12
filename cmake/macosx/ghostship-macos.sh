#!/bin/bash

SNAME="$(dirname $0)"
export SHIP_HOME="$HOME/Library/Application Support/com.shipofharkinian.ghostship"
export SNAME
export RESPATH="${SNAME%/MacOS*}/Resources"
export LIBPATH="${SNAME%/MacOS*}/Frameworks"
export DYLD_FALLBACK_LIBRARY_PATH="$LIBPATH"

arch_name="$(uname -m)"
launch_arch="arm64"
if [ "${arch_name}" = "x86_64" ] && [ "$(sysctl -in sysctl.proc_translated)" != "1" ]; then
	launch_arch="x86_64"
fi

arch -${launch_arch} "$RESPATH"/Ghostship
exit
