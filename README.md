# Power_steering

## Prerequisite and Installation
OMPT is used for power collection and adjusting frequency
To use with icc:
The following llvm-openmp runtime needs to be installed and replacing the official OpenMP runtime from icc
https://github.com/OpenMPToolsInterface/LLVM-openmp/tree/towards_tr4
Make sure you put -DLIBOMP_OMPT_SUPPORT=TRUE in cmake when building the runtime
 
To use with clang:
Clang/LLVM and the above openmp runtime needs to be installed

## Development
Build all power/freq related sources and the ompt_power.h file into one library libpowersteering.so

When you build you application, link the application with the library. 
icc or clang
clang -fopenmp axpy.c -L<llvm-openmp-lib-location> -lomp  -lpowersteering -o axpy

ldd axpy to check the executable uses the right library

