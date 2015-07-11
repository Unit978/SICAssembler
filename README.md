# SICAssembler

**OVERVIEW**

A Simplified Instruction Computer Assembler written in C++. This mini-project was part of my coursework. The code works in Linux environments only.

The SIC Assembler simulates a terminal prompt where the user can input commands for the SIC machine to process.

**COMMANDS**

Commands available: Load, Execute, Debug, Dump, Help, Assemble, Directory, Exit.

- `load [filepath]` Loads the object file produced by the Assemble command. filepath = object file path.
- `execute` Executes the loaded assembly source file.
- `debug` Not Implemented
- `dump [start][end]` Displays the values in the memory locations between start & end (in hexadecimal) of the SIC 
machine
- `help` Shows the list of commands available.
- `assemble [filepath]` Assembles the assembly source code for execution. filepath = assembly source path (.asm)
- `directory` Shows the current directory content. Equivalent to Linux's ls command.
- `exit`  Terminates the simulation.

**INPUT**

The Assembler should take any valid SIC source code.
The sample assembly source code (source.asm) copies the contents of a file to another. 

**OUTPUT**

The program will generate an object file, intermediate file, and a listing file. 
* The assembler uses the intermediate file for its own purposes (2 pass assembler). 
* The listing file is the report of the assemblage (such as reporting errors.). 
* The object file is the machine translation of the source code and it is fed into the SIC machine for execution.

The output of sample source is written to the file dev05 (generated by the SIC machine).

**MISC**

The files used for copying are devf1 & dev05.
