#!/usr/bin/env python3
"""Copy a mod's manifest.json, setting code_version and api_version to the versions of the headers it is built with."""

import argparse
import json
import re
import sys

VERSIONS = [("code_version", "GS_CODE_VERSION"), ("api_version", "GS_API_VERSION")]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--api", required=True, help="path to src/port/api/api.h")
    ap.add_argument("--src", required=True, help="manifest.json of the mod")
    ap.add_argument("--out", required=True, help="manifest.json to write")
    args = ap.parse_args()

    with open(args.api) as f:
        header = f.read()
    with open(args.src) as f:
        manifest = json.load(f)

    for key, define in VERSIONS:
        m = re.search(r"^#define\s+%s\s+(\d+)" % define, header, re.MULTILINE)
        if m is None:
            sys.exit("mod_manifest: %s is not defined in %s" % (define, args.api))
        manifest[key] = int(m.group(1))

    with open(args.out, "w") as f:
        json.dump(manifest, f, indent=4)
        f.write("\n")


if __name__ == "__main__":
    main()
