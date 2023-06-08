import pandas as pd
import re
import numpy as np 
from itertools import islice

# Tool used to debug prog counters for the registers
# generated using PIN. Using this tool we can check the
# top instructions that generate the register writes
# and then correlate them to what should happen.

def replace_whitespace(string):
    return re.sub(r'\s+', ' ', string)

# read the files
file1 = open('trace', 'r')

trace = file1.readlines()
prog_counters = {}
for trace_vals in trace:
    
    if 'IP' in trace_vals:
        pc = trace_vals.split()[1][3:]
        if pc in prog_counters:
            prog_counters[pc] += 1
        else:
            prog_counters[pc] = 1

print(len(prog_counters))

for k,v in sorted(prog_counters.items(), key=lambda x:x[1]):
    print(k, v)
