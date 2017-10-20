# TRISK_CPU
TRISK_CPU stands for Toy RISC CPU. This project is an emulator and assembler for a simple/toy 8-bit RISC designed in 2016.

### Building

Use CMAKE. On linux, the lazy out there can run `./lmr`

### Usage

`tas` is the assembler.

`tem` is the emulator.

To assemble and run the program:

```
./tas <input assembly file> <output binary file>

./tem <input binary file> <output binary file>
```

`<output binary file>` contains the final state of RAM after the program has finished execution.



Sample programs can be found in `sample_programs/`


### Instruction Set

```
NOP				Does nothing ("No Operator"). Fetches next instruction.
HALT			Halts the CPU.
SET	<X> <Y>		[X = Y] Sets register X to register Y.
PCL	<X>			[PC = X iff L == 1] Sets program counter to address pointed to by register X (which can be a label) iff L flag == 1.
PCO <X>			[PC = X iff O == 1] Sets program counter to address pointed to by register X iff O flag == 1.
PCS <X>			[PC = X iff S == 1] Sets program counter to address pointed to by register X iff S flag == 1.
LDI <X> <VAL>	[X = (*(PC++))] Sets register X to whatever byte VAL is.
LD	<X> <Y>		[X = *Y] Sets register X to whatever byte in RAM at location Y is.
ADD <X> <Y>		[X += Y]
SUB <X> <Y>		[X -= Y]
RSHIFT <X> <Y>	[X >>= Y]
NOT <X>			[X = ~X]
JMP	<X>			[PC = X] Sets program counter to address pointed to by register X (unconditional jump).
PCC <X>			[PC = X iff C = 1] Sets program counter to address pointed to by register X iff C flag == 1.
PCZ <X>			[PC = X iff Z = 1] Sets program counter to address pointed to by register X iff Z flag == 1.
AND <X> <Y>		[X &= Y]
OR <X> <Y>		[X |= Y]
CMP <X> <Y>		[X - Y] (no store -- only sets flags)
ST <X> <Y>		[*X = Y] Sets RAM pointed to by register X to Y.
```

Refer to beginning of `src/assembler/assembler.cpp` for complete assembly syntax and notes.
