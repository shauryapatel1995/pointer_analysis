#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string_view> 
#include <thread>
#include <fstream>
#include <deque>
#include <sstream>
#include <mutex>
#include <unordered_map>
#include <set>
#include <regex>
#include <sstream>

bool operator <(const timespec& lhs, const timespec& rhs)
{
    if (lhs.tv_sec == rhs.tv_sec)
        return lhs.tv_nsec < rhs.tv_nsec;
    else
        return lhs.tv_sec < rhs.tv_sec;
}

struct read_line_args {
    char *fd_string;
    char *line_buffer;
    struct timespec record_time; 
};

struct pagefault_values {
    u_int64_t page_address;
    u_int64_t pagecache_address;
    timespec record_time; 
};

struct mem_values {
    u_int64_t PC;
    u_int64_t source;
    u_int64_t reg_no; 
    u_int64_t reg_value;
    timespec record_time; 
};


void print_mem_value(struct mem_values *val) {
    std::cout << "Reg no: " << val->reg_no << " Reg value: " << std::hex << val->reg_value << std::dec << " Time: " << val->record_time.tv_sec << ":" << val->record_time.tv_nsec << std::endl;
}

void print_pagefault(struct pagefault_values *val) {
    std::cout << "Addr: " << std::hex << val->page_address << std::dec << " Time: " << val->record_time.tv_sec << "." << val->record_time.tv_nsec << std::endl;
}

volatile int end_analysis = 0;
std::deque<pagefault_values *> pagefaults; 
std::deque<mem_values *> mem_trace_values; 
std::mutex pagefaults_lock; 
std::mutex mem_trace_lock;
std::unordered_map<u_int64_t, u_int64_t> active_registers; 
std::unordered_map<u_int64_t, u_int64_t> active_reg_values;
std::unordered_map<u_int64_t, u_int64_t> active_reg_values_source;
std::unordered_map<u_int64_t, u_int64_t>  reg_prog_counters;

// Update the current with the source if the memory read is associated to this IP.
struct mem_values * curr_mem_val;
int missed_instructions = 0;

/*
 * Parse a memory access and update the current register state.
 */
void update_registers_state(struct mem_values *curr_mem_trace_val) {
    if (active_registers.count(curr_mem_trace_val->reg_no)) {
        u_int64_t value = active_registers[curr_mem_trace_val->reg_no]; 
        // Decrease a ref to an alive value and remove it if necessary.
        active_reg_values[value]--;
        if (active_reg_values[value] == 0) {
                // Erase values from both source map and values map.
                active_reg_values.erase(value);
                active_reg_values_source.erase(value);
        }
        active_registers[curr_mem_trace_val->reg_no] = curr_mem_trace_val->reg_value;
                
        // Check if value already exists in another register, then just add a ref to it.
        // If not add a new ref to the value.
        if (active_reg_values.count(curr_mem_trace_val->reg_value)) 
            active_reg_values[curr_mem_trace_val->reg_value]++;
        else
            active_reg_values[curr_mem_trace_val->reg_value] = 1;

        // Add source data for the value. We assume latest source is the source 
        // of the value.
        active_reg_values_source[curr_mem_trace_val->reg_value] = curr_mem_trace_val->source;
    } else {
        active_registers[curr_mem_trace_val->reg_no] = curr_mem_trace_val->reg_value;
        active_reg_values_source[curr_mem_trace_val->reg_value] = curr_mem_trace_val->source;
        if (active_reg_values.count(curr_mem_trace_val->reg_value))
            active_reg_values[curr_mem_trace_val->reg_value]++;
        else 
            active_reg_values[curr_mem_trace_val->reg_value] = 1;
    }
}

/*
 * Merge the two traces and calculate required stats.
 */
void merge_traces_and_collect_stats(void *args) {
    struct mem_values *curr_mem_trace_val = NULL; 
    struct pagefault_values *curr_pagefault_val = NULL; 

    int predicted_pointer_pagefaults = 0; 
    int unpredicted_pointer_pagefaults = 0; 
    int predicted_non_pointer_pagefaults = 0;
    int unpredicted_non_pointer_pagefaults = 0;
    int nothing_to_trace = 0;
    while (1) {
        // sleep here.
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Size of memory trace " << mem_trace_values.size() << " Pagefault trace size " << pagefaults.size() << std::endl;
        struct timespec curr;
        clock_gettime(CLOCK_MONOTONIC, &curr);

        if (mem_trace_values.size() > 0) {
	    std::cout << "Acquiring lock (merge)\n";
            mem_trace_lock.lock();
            if (!mem_trace_values.empty()) 
		std::cout << "mem trace values not empty\n";
                curr_mem_trace_val = mem_trace_values.front(); 
            mem_trace_lock.unlock();
	    std::cout << "Freeing lock (merge) \n";
        }

        if (!curr_mem_trace_val) {
            nothing_to_trace++;
            std::cout << "Nothing to trace\n";
            /*if (nothing_to_trace > 15) {
                std::cout<<"nothing to trace \n"; 
                end_analysis = 1;
                return;
            }*/
            continue; 
        }

        if (pagefaults.size() > 0) {
            pagefaults_lock.lock();
            if (!pagefaults.empty()) 
                curr_pagefault_val = pagefaults.front();
            pagefaults_lock.unlock();
        }
        
        if (!curr_pagefault_val) {
            // Only keep trace values for 2 mins and process the rest.
            while (curr_mem_trace_val && curr_mem_trace_val->record_time.tv_sec < curr.tv_sec - 120) {
                update_registers_state(curr_mem_trace_val);
                mem_trace_lock.lock();
                free(curr_mem_trace_val);
                curr_mem_trace_val = NULL;
                mem_trace_values.pop_front();
                if (!mem_trace_values.empty()) {
                    curr_mem_trace_val = mem_trace_values.front();
		}
                mem_trace_lock.unlock();
            }
            continue; 
        }

        std::cout << "Analyzing time of first is: " << curr_mem_trace_val->record_time.tv_sec << " Curr time is :" << curr.tv_sec << std::endl;
        std::cout << "Time of pagefault is: " << curr_pagefault_val->record_time.tv_sec << std::endl;
        while (curr_mem_trace_val && curr_mem_trace_val->record_time.tv_sec < curr.tv_sec - 10 && curr_pagefault_val) {
            // Pagefault is in future update the state we know.
            if (curr_mem_trace_val->record_time < curr_pagefault_val->record_time) {
                update_registers_state(curr_mem_trace_val);
                mem_trace_lock.lock();
                //print_mem_value(curr_mem_trace_val);
                mem_trace_values.pop_front();
                free(curr_mem_trace_val);
                curr_mem_trace_val = NULL;
                if (!mem_trace_values.empty()) 
                    curr_mem_trace_val = mem_trace_values.front();
                mem_trace_lock.unlock();
            } else {
                // Pagefault encountered check current state.
                if (active_reg_values.count(curr_pagefault_val->page_address)) {

                    if (active_reg_values_source.count(curr_pagefault_val->page_address)) 
                        std::cout << "Source: " << active_reg_values_source.at(curr_pagefault_val->page_address) 
                        << " Dest: " << curr_pagefault_val->page_address << std::endl;
                    
                    if (curr_pagefault_val->pagecache_address)
                        predicted_pointer_pagefaults++;
                    else
                        unpredicted_pointer_pagefaults++;
                } else {
                    if (curr_pagefault_val->pagecache_address)
                        predicted_non_pointer_pagefaults++;
                    else
                        unpredicted_non_pointer_pagefaults++;
                }

                /*
                 * Debug code for debugging the Program counters of faults.
                    if (non_pointer_pagefaults % 100 == 1 || pointer_pagefaults % 100 == 1) {
                     std::cout << "Pagefault address: " << std::hex <<curr_pagefault_val->page_address << std::endl;
                    for (auto const &pair: active_reg_values) {
                         std::cout << std::hex << pair.first << ": " << pair.second << std::endl;
                    }
                    std::cout << "Program counters" << std::endl; 
                    for (auto const &pair: reg_prog_counters) {
                         std::cout << pair.first << ": " << std::hex << pair.second << std::endl;
                    }


                } */
                pagefaults_lock.lock();
                //print_pagefault(curr_pagefault_val);
                pagefaults.pop_front();
                free(curr_pagefault_val);
                curr_pagefault_val = NULL;
                if (!pagefaults.empty())
                    curr_pagefault_val = pagefaults.front();
                pagefaults_lock.unlock();
            }
        }
    

        std::cout << "Pointer based pagefaults : " << predicted_pointer_pagefaults + unpredicted_pointer_pagefaults << " Non pointer based pagefaults : " << predicted_non_pointer_pagefaults + unpredicted_non_pointer_pagefaults << std::endl;
        std::cout << "Predicted pointer faults: " << predicted_pointer_pagefaults << " Unpredicted pointer faults: " << unpredicted_pointer_pagefaults << std::endl;
        

}
}

// TODO(shaurp): Check if we can change this to string_view
/*
 * Parse a pagefault and save it in pagefault stream.
 */
void parse_pagefault(std::string pagefault_str) {
    std::string data;
    // TODO(shaurp): Make this application agnostic
/*    if (pagefault_str.find("specperl") == std::string::npos){
        return; 
    }*/
    std::regex time_pattern(R"(\d+\.\d+)");
    std::smatch matchResult;
    struct pagefault_values *pagefault = (struct pagefault_values *) malloc(sizeof(pagefault_values));
    std::stringstream ss(pagefault_str);
    while(ss >> data) { 
        if (data.find("vpage") != std::string::npos) {
            std::string page_num = data.substr(8);
            pagefault->page_address = std::stoull(page_num, 0, 16);
        } else if (data.find("page=") != std::string::npos) {
            std::string pte_num = data.substr(7);
            if (pte_num.compare("0") == 0) {
                pagefault->pagecache_address = 0;
            } else {
                pagefault->pagecache_address = 1;
            }
        } // get timestamp: 
          else if (std::regex_search(data,matchResult,time_pattern)) {
            std::string::size_type pos = matchResult.position(0);
            if (data.substr(0,pos).compare("") == 0 || data.substr(pos + 1).compare("") == 0) {
                //std::cout << "Pagefault freed: " << data.substr(0,pos)<< std::endl;
                free(pagefault);
                return;
            }

            pagefault->record_time.tv_sec = stol(data.substr(0, pos));
            pagefault->record_time.tv_nsec = stol(data.substr(pos + 1));

            if(pagefault->record_time.tv_sec == 0) 
                std::cout << "Zero pagefault time" << std::endl;
        }
    }
    pagefaults_lock.lock();
    pagefaults.push_back(pagefault);
    pagefaults_lock.unlock();

    return;
}

/*
 * Parse PIN memory access trace string.
 */
void parse_mem_read(std::string mem_read) {
    // Function to parse mem_read, it should be associated with the last register read.
    if (mem_read.find("R") != std::string::npos && curr_mem_val) {
        unsigned long long PC = std::stoull(mem_read.substr(0, 14), 0, 16);

        if(PC == curr_mem_val->PC) { 
            curr_mem_val->source = (std::stoull(mem_read.substr(mem_read.find("R") + 2), 0, 16) &  ~((1 << 12)-1));
        }
    }
}

/*
 * Parse PIN register load trace string.
 */
void parse_mem_trace(std::string mem_trace) {
    std::string data;
   
    struct mem_values *mem_values = (struct mem_values *) malloc(sizeof(struct mem_values));
    std::stringstream ss(mem_trace);
    while(ss >> data) {
        if (data.find(".") != std::string::npos) {
            if (data.find_first_not_of("0123456789.") != std::string::npos) {
                free(mem_values);
                missed_instructions++;
                return;
            }
            std::string::size_type pos = data.find('.');
            if (data.substr(0,pos).compare("") == 0 || data.substr(pos + 1).compare("") == 0) {
                free(mem_values);
                missed_instructions++;
                return;
            }

            mem_values->record_time.tv_sec = stol(data.substr(0, pos));
            mem_values->record_time.tv_nsec = stol(data.substr(pos + 1));
            if(mem_values->record_time.tv_sec == 0) {
                free(mem_values);
                missed_instructions++;
                return;
            }

            if (mem_values->record_time.tv_sec > 10000000) {
                std::cout << "FROM DATA" << std::endl;
                std::cout << data << std::endl;
                std::cout << "PRINTING VALUE" << std::endl;
                print_mem_value(mem_values);
            }
        } else if (data.find("Reg:") != std::string::npos) {
            if(data.size() < 7) {
                free(mem_values);
                missed_instructions++;
                return;
            }
             
            if (data.substr(4).find_first_not_of("0123456789abcdefABCDEF") == std::string::npos) {
                if (data.substr(4).size() > 16) {
                    free(mem_values);
                    //std::cout << "Data issue in register" << data << std::endl;
                    missed_instructions++;
                    return;
                }
                //std::cout << data << std::endl;
                mem_values->reg_value = (std::stoull(data.substr(4), 0, 10) &  ~((1 << 12)-1));
                //std::cout << mem_values->reg_value << std::endl;
                
            } else {
                //std::cout << "Data has other digits" << std::endl;
            }
        } else if (data.find("RegNo:") != std::string::npos) {
            if(data.size() < 7) {
                free(mem_values);
                missed_instructions++;
                return;
            }
            if (data.substr(6).find_first_not_of("0123456789") == std::string::npos && data.substr(6).size() < 10) {
                mem_values->reg_no = std::stoi(data.substr(6));
            } else {
                free(mem_values);
                missed_instructions++;
                return;
            }
                
        } else if (data.find("IP") != std::string::npos) {
                mem_values->PC = std::stoull(data.substr(5), 0, 16);
        }
    }
//    std::cout <<"Acquiring lock (parse mem)\n";
    mem_trace_lock.lock();
    // print_mem_value(mem_values);
    if (mem_values->record_time.tv_sec > 10000000) {
        std::cout << "PRINTING VALUE " << mem_trace << std::endl;
        print_mem_value(mem_values);
    }
    // Update the latest register load processed.
    curr_mem_val = mem_values;
    mem_trace_values.push_back(mem_values);
    mem_trace_lock.unlock();
//    std::cout << "Freeing lock (parse mem) \n";
}

void read_lines_for_pagefault(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::in);
    const size_t buffer_size = 8192; // Adjust based on your needs
    std::vector<char> buffer(buffer_size + 1); // Extra space for null terminator
    std::string partial_line;
    while (file.read(buffer.data(), buffer_size) || file.gcount()) {
        buffer[file.gcount()] = '\0'; 
        std::string current_chunk = partial_line + buffer.data();
        std::istringstream chunk_stream(current_chunk);
        std::string line;
	// might change this threshold later
        while(pagefaults.size() > 10000){
            std::this_thread::yield();
        }
        while (std::getline(chunk_stream, line)) {
            if (!chunk_stream.eof()) {
                line.append("\n");
                parse_pagefault(line);
            } else {
                // This line is incomplete, store it for the next chunk
                partial_line = line;
            }
        }
    }
}

void read_lines_for_memtrace(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::in);
    const size_t buffer_size = 8192;
    std::vector<char> buffer(buffer_size + 1); // Extra space for null terminator
    std::string partial_line;
    //std::ifstream file;
    while (file.read(buffer.data(), buffer_size) || file.gcount()) {
        buffer[file.gcount()] = '\0'; 
        std::string current_chunk = partial_line + buffer.data();
        std::istringstream chunk_stream(current_chunk);
        std::string line;
	// might change this threshold later
        while(mem_trace_values.size() > 10000){
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::this_thread::yield();
        }
        while (std::getline(chunk_stream, line)) {
            if (!chunk_stream.eof()) {
                if (line.find("eof") != std::string::npos) {
                    std::cout << "eof" << std::endl;
                    return;
                }
                line.append("\n");
                if (line.find("Reg") != std::string::npos) {
                    parse_mem_trace(line);
                } else {
                    parse_mem_read(line);
                }
            } else {
                partial_line = line;
            }
        }
    }
}


int main(int argc, char *argv[]) {
    char * line_buffer = (char *) malloc(4096);
    char * mem_buffer = (char *) malloc(4096);

    pthread_t thread_id, thread_id_2;
    std::cout << "Main thread running \n";
    std::cout << argv[1] << " " << argv[2] << std::endl;
    struct read_line_args args = {argv[1], line_buffer};
    std::thread t1(read_lines_for_pagefault, std::string(argv[1]));

    std::thread t2(read_lines_for_memtrace, std::string(argv[2]));
    std::thread t3(merge_traces_and_collect_stats, &args);
    
    t3.join();
    t2.join();
    t1.join();
    exit(EXIT_SUCCESS);
}


