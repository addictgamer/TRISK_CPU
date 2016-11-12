# TRISK_CPU
An emulator and assembler for a simple 8-bit RISC designed in Dr Tak's Computer Architecture &amp; Assembly Class

### Building

Use CMAKE. On linux, the lazy out there can run `./lm`

### Usage

`tas` is the assembler.

`tem` is the emulator.

To assemble and run the program:

```
./tas <input assembly file> <output binary file>

./tem <input binary file> <output binary file>
```

`<output binary file>` contains the final state of RAM after the program has finished execution.

Instruction set can be found at the top of `src/assembler/assembler.cpp`

Sample programs can be found in `sample_programs/`
