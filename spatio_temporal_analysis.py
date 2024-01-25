# This analyzes a trace collected from CheriBSD. 
# Trace format - 
# CP analysis: Faulting page addr, faulting page offset. 
# CP analysis: Pointer inside page. 
# CP analysis: Next faulting page. 
# This analysis answers the question of whether 
# running CP at majorfault is a good idea. 
# Future work is to extend this work to minorfaults.

# We want to look at future pagefaults from current pagefault.
pagefault_stream = dict()
predicted = 0
num_future_pagefaults = 128
def collect_pagefault(pagefault_str, fault_index):
    # Convert pagefault string to pagefault val.
    trace = pagefault_str.split()
    faulting_page = trace[9][:-1]
    faulting_addr = trace[12]
    faulting_page = int(faulting_page, 16)
    # print(faulting_page, faulting_addr)
    if faulting_page in pagefault_stream:
        pagefault_stream[faulting_page].append(fault_index)
    else:
        pagefault_stream[faulting_page] = []
        pagefault_stream[faulting_page].append(fault_index)
    #pagefault_stream.append((int(faulting_page, 16), 
    #    int(faulting_addr, 16)))

def collect_pointers(pointer_str, fault_index):
    global predicted
    global num_future_pagefaults
    # Convert pointer string to pointer val
    # Check if the pointer exists in the future 
    # pagefault stream.
    pointer = int(pointer_str.split()[-1], 16)
    pointer_page = pointer & ~(4096 - 1)
    if pointer_page in pagefault_stream:
        # Look at its access history.
        for fault_index_history_val in pagefault_stream[pointer_page]:
            if fault_index_history_val > fault_index and fault_index_history_val < fault_index + num_future_pagefaults:
                    predicted += 1
                    break


def main():
    global predicted
    print("Running analysis")   
    print("Creating pagefault history")
    fault_index = -1
    # This counter is used to print what is the offset of the pointer 
    # on the page.
    pointer_counter = 0
    with open('/data1/ll-cp-analysis-random') as trace:
        fault = trace.readline()
        while fault:
            if "Faulting" in fault:
                fault_index += 1
                collect_pagefault(fault, fault_index)
            fault = trace.readline()
    print("Total pagefaults: ", fault_index)
    print("Pagefault history created, total unique pagefaults: ", 
            len(pagefault_stream))
    print("Analyzing pointers that caused pagefaults")
    fault_index = -1
    # Parse the file
    with open('/data1/ll-cp-analysis-random') as trace:
        fault = trace.readline()
        while fault:
            if "Faulting" in fault:
                #TODO(shaurp): Reset the pointer counter here.
                #TODO(shaurp): Write faulting addr to file.
                fault_index += 1
            elif "Address" in fault:
                #TODO(shaurp): Print fault index from inside here.
                #TODO(shaurp): Update script to read the offset from the trace. 
                collect_pointers(fault, fault_index)
            else:
                print("Trace is incorrect", fault)
            fault = trace.readline()

    print("Total possible prediceted major faults: ", predicted)
if __name__ == "__main__":
    main()


