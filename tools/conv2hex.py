import sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("FILE", help="the name of the file that you wish to dump", type=str)
args = parser.parse_args()

try:
    with open(args.FILE, "rb") as f:
        n = 0
        b = f.read(16)

        while b:
            s1 = ",".join([f"0x{i:02x}" for i in b]) # hex string
            #s1 = s1[0:23] + " " + s1[23:]          # insert extra space between groups of 8 hex values

            s2 = "".join([chr(i) if 32 <= i <= 127 else "." for i in b]) # ascii string; chained comparison

            print(s1 + ",", end="")

            n += 1
            b = f.read(16)
        print()
        
except Exception as e:
    print(__file__, ": ", type(e).__name__, " - ", e, sep="", file=sys.stderr)