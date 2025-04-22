import os

EXECUTABLE="./test_res"
INPUT="tmp/in.txt"
OUTPUT="tmp/out.txt"

args = (EXECUTABLE, "<", INPUT, ">", OUTPUT)
cmd = " ".join(args)

output = os.popen(cmd).read()
