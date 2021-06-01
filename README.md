# Processor-Simulator
This was a coursework done for University.

I created a simulator for a 5-stage multi-cycle MIPS processor, with three different cache models.
I also tested different types of caches:directly-mapped, fully associative and 2-way set associative.

mipssim.c is the main file, where the cpu is simulated.
memory_hierarchy.c is the memory hierarchy implementation, with the caches.
parser.h is for the reading and parsing of input files.

The "memfile-*.txt" consists of the MIPS instructions and data.
the "regfile_*.txt" constains the state of the registers at the start of execution.

In order to run the processor, run the following command (after compiling them):
./mipssim 1024 1 memfile-simple.txt regfile.txt
where 1024 indicates the cache is enabled and set to a size of 1024 bytes.
1 configures the cache type as direct mapped, 2 is fully associative and 3 is 2-way set associative.