My first time writing a compiler - mostly to improve my knowledge of low level coding and C++.
Guided by this series from Pixeled: https://www.youtube.com/watch?v=vcSijrRsrY0&list=PLUDlas_Zy_qC7c5tCgTMYq2idyyT241qs&index=1
and berkeley's CS164 course https://inst.eecs.berkeley.edu/~cs164/fa22/schedule.html\ 

Will not be trying to make an optimal and majorly useable language, but just for learning purposes. Also this currently only runs on linux. 

To build on your linux machine, make sure you have nasm (assembler) and ld (GNU linker) installed and cloned this repo, run:

- cd hydrogen
- mkdir build
- cmake -S . -B build
- cmake --build build

To run the test.hy file with any modifications in the file you may run;

- cd build
- ./hydro ../test.hy <---updates with your changes
- ./out
- echo $? <----prints output value of test.hy onto terminal

Things left to finish before i am satisfied with this project for now:
  - Floating point arithmetic
  - Basic functions
  - Basic pointers 
  - Maybe something wack
  - Maybe a quick video or write up
