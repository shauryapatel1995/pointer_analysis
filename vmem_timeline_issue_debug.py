import pandas as pd
import re
import numpy as np 
from itertools import islice
def replace_whitespace(string):
    return re.sub(r'\s+', ' ', string)

# read the files
file1 = open('analysis_pass', 'r')
file2 = open('output_pass', 'r')

# Create a timeline of all virtual addresses.
# Each timeline contains loads where the value
# of the address was loaded into a register. 
# and a pagefault of the same address. 
# Time complexity O(loads) space O(pagefaults)
addrs = file2.readlines()
addr_values = []
for addr in addrs:
    if '0x' in addr:
        addr_values.append(int(addr[2:], 16))

print(addr_values)

analysis_trace = file1.readlines()
count = 0
vmem_timelines = dict()
prev_address = 0
for log in analysis_trace:
    if 'Reg' not in log and 'Addr' not in log:
        continue

    if 'Reg' in log: 
        # Do stuff for register addresses.
        register_values = log.split()
        address = int(register_values[5], 16)
        if address in vmem_timelines and address in addr_values:
            vmem_timelines[address].append(log)
        elif address in addr_values:
            vmem_timelines[address] = []
            vmem_timelines[address].append(log)
    else:
        # This will now contain both registers and pagefaults.
        trace = log.split()
        address = int(trace[1], 16)
        # Major pagefaults might show up twice in the trace.
        if address == prev_address:
            continue
        if address in vmem_timelines and address in addr_values:
           vmem_timelines[address].append(log)
        elif address in addr_values:
            vmem_timelines[address] = []
            vmem_timelines[address].append(log)
        prev_address = address
        count += 1
print("Starting analysis on timeline")
   
n = 100
preceeding_same_second=0
not_preeceding = 0
for idx, (k,v) in enumerate(vmem_timelines.items()):
    print("Address value: ", k)
    print("Timeline: ")
    for value in v:
        print(value)
