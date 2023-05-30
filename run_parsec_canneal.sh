	../vmem_timeline_stream <(sudo ..
/../jackson-linux/tools/perf/perf record -e probe:do_swap_page_L46 --clockid CLOCK_MONOTONIC -aR | ../..
/jackson-linux/tools/perf/perf script) <(parsecmgmt -a run -p parsec.canneal -i simlarge -s "sudo cgexec
 -g memory:trial /home/shaurya/pin/pin-3.26-98690-g1fc9d60e6-gcc-linux/pin -t /home/shaurya/pin/pin-3.26
-98690-g1fc9d60e6-gcc-linux/source/tools/ManualExamples/obj-intel64/pinatrace.so --")
