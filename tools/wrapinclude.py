import sys

if len(sys.argv) > 1:
   f = open(sys.argv[1])
else:
   f = sys.stdin 

try:
    Lines = f.readlines()

    for line in Lines:
        if len(line.strip()) > 0:
            print("#include \"" + line.strip() + "\"")
        
except Exception as e:
    print(__file__, ": ", type(e).__name__, " - ", e, sep="", file=sys.stderr)