My first time writing a compiler - mostly to improve my knowledge of low level coding and C++.
Guided by this series from Pixeled: https://www.youtube.com/watch?v=vcSijrRsrY0&list=PLUDlas_Zy_qC7c5tCgTMYq2idyyT241qs&index=1
and berkeley's CS164 course https://inst.eecs.berkeley.edu/~cs164/fa22/schedule.html\ 

Will not be trying to make an optimal and majorly useable language, but just for learning purposes. Also this currently only runs on linux. 

To build on your linux machine, make sure you have nasm (assembler) and ld (GNU linker) installed and cloned this repo, run:

```
cd hydrogen
mkdir build
cmake -S . -B build
cmake --build build
```

To run the test.hy file with any modifications in the file you may run:
```
cd build
./hydro ../test.hy
./out
echo $?
```

I am currently done working on this project at the moment, but I made a separate branch for code I was testing before I moved on. If I ever come back to this, these will be the first things I do:
  - Floats work decently, but there are still some bugs with the precedence climbing algo and its interaction with floats + int combination arithmetic
  - Pointers are broken, and despite the basic logic being there and making sense, something about grabbing the address of previous variables I put on the stack is not working. 
  - Functions are not implemented, but scopes are, so I imagine its a matter of just defining proper tokens for a function header and then giving that function node in the AST a scope node to work with, similar to how if statements work.

Thanks for viewing :) 
