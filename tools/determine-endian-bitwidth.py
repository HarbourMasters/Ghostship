import struct
import sys


print("--endian " + sys.byteorder + " --bitwidth " + str( 8 * struct.calcsize("P")), end="")