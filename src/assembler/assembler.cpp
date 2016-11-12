/* Copyright Ciprian Ilies 2016 */

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <iterator>
#include <sstream>

/*
 * This is a *very* basic assembler for the toy processor 8-bit RISC CPU.
 *
 * Instruction set:
 *		NOP				Does nothing ("No Operator"). Fetches next instruction.
 *		HALT			Halts the CPU.
 *		SET	<X> <Y>		[X = Y] Sets register X to register Y.
 *		PCL	<X>			[PC = X iff L == 1] Sets program counter to address pointed to by register X (which can be a label) iff L flag == 1.
 *		PCO <X>			[PC = X iff O == 1] Sets program counter to address pointed to by register X iff O flag == 1.
 *		PCS <X>			[PC = X iff S == 1] Sets program counter to address pointed to by register X iff S flag == 1.
 *		LDI <X> <VAL>	[X = (*(PC++))] Sets register X to whatever byte VAL is.
 *		LD	<X> <Y>		[X = *Y] Sets register X to whatever byte in RAM at location Y is.
 *		ADD <X> <Y>		[X += Y]
 *		SUB <X> <Y>		[X -= Y]
 *		RSHIFT <X> <Y>	[X >>= Y]
 *		NOT <X>			[X = ~X]
 *		JMP	<X>			[PC = X] Sets program counter to address pointed to by register X (unconditional jump).
 *		PCC <X>			[PC = X iff C = 1] Sets program counter to address pointed to by register X iff C flag == 1.
 *		PCZ <X>			[PC = X iff Z = 1] Sets program counter to address pointed to by register X iff Z flag == 1.
 *		AND <X> <Y>		[X &= Y]
 *		OR <X> <Y>		[X |= Y]
 *		CMP <X> <Y>		[X - Y] (no store -- only sets flags)
 *		RAM <X> <Y>		[*X = Y] Sets RAM pointed to by register X to Y.
 *
 *	To allocate data:
 *		BYTE <x>
 *	Data section should probably be at the end of your source file.
 *
 *	Assembler does not support labels (yet).
 *
 *	Keep in mind that you only have 4 8-bit registers to play with, and 256 bytes of RAM.
 *	Registers:
 *		A
 *		B
 *		C
 *		D
 *
 *	Must have whitespace between keywords, symbols, literals, etc.
 *	Integer literals must be specified as decimal numbers. Hexadecimal and binary notation are not supported.
 *	Specify registers as numbers letters A - D or numbers 0 - 3: LDI A 127 for example
 *
 *	You should end your program with halt, otherwise it will loop through RAM forever, and possibly might have unintended consequences.
 *
 *	TODO: Comments support.
 *
 *	TODO: Test labels.
 *
 */




/*
 * Instructions will be implemented like so:
 * * Array of structs which has the name and the function pointer.
 * * Read in instruction from source file, lookup instruction in instruction array/table, call its function pointer.
 * * Function pointer reads in whatever optional parameters. Also validates instruction usage.
 * If at any time an illegal instruction is encountered or misformatted source code, error dump the user & abort.
 */

static const uint16_t RAM_SIZE = 256; //How much program memory we have (8-bit CPU/RAM).
static const uint16_t NUM_REGISTERS = 4;

struct Instruction
{
	std::string name;
	uint16_t instruction_size; //Size of instruction in this architecture in bytes.
	uint8_t num_parameters;

	Instruction()
	{
		name = "NULL";
		instruction_size = 1;
		num_parameters = 0;
	}
	virtual ~Instruction() = 0;

	//Returns number of bytes written. 0 on error.
	virtual int parse(uint8_t& address, uint8_t* memory, int = 0, int = 0) = 0;
};

inline Instruction::~Instruction() { } //magic.


// ***** INSTRUCTION IMPLEMENTATIONS *****

//0x00 0000_0000 -- nop
struct irNOP : public Instruction
{
	irNOP()
	{
		name = "NOP";
		instruction_size = 1;
		num_parameters = 0;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int = 0, int = 0)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		memory[address] = 0x00;

		++address;

		return instruction_size;
	}
};

//0x01 0000_0001 -- halt
struct irHalt : public Instruction
{
	irHalt()
	{
		name = "HALT";
		instruction_size = 1;
		num_parameters = 0;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int = 0, int = 0)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		memory[address] = 0x01;

		++address;

		return instruction_size;
	}
};

//0x5? 0101_xxyy -- x = y
struct irSet : public Instruction
{
	irSet()
	{
		name = "SET";
		instruction_size = 1;
		num_parameters = 2;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int reg2)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}
		if (reg2 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg2 << "\".\n";
			return 0;
		}

		memory[address] = 0x50 + (reg1 << 2) + (reg2);
		++address;

		return instruction_size;
	}
};

//0x6? 0110_00xx -- PC = x iff L=1
struct irPCL : public Instruction
{
	irPCL()
	{
		name = "PCL";
		instruction_size = 1;
		num_parameters = 1;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int = 0)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}

		memory[address] = 0x60 + (reg1);
		++address;

		return instruction_size;
	}
};

//0x6? 0110_01xx -- PC = x iff O=1
struct irPCO : public Instruction
{
	irPCO()
	{
		name = "PCO";
		instruction_size = 1;
		num_parameters = 1;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int = 0)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}

		memory[address] = 0x64 + (reg1);
		++address;

		return instruction_size;
	}
};

//0x6? 0110_10xx -- PC = x iff S=1
struct irPCS : public Instruction
{
	irPCS()
	{
		name = "PCS";
		instruction_size = 1;
		num_parameters = 1;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int = 0)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}

		memory[address] = 0x68 + (reg1);
		++address;

		return instruction_size;
	}
};

//0x6? 0110_11xx -- X = (*(PC++))
struct irLDI : public Instruction
{
	irLDI()
	{
		name = "LDI";
		instruction_size = 2; //instruction + 1 operands
		num_parameters = 2;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int value)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}

		//First the actual instruction's code.
		memory[address] = 0x6C + reg1;
		++address;
		//Then the parameters to the instruction.
		memory[address] = value;
		++address;

		return instruction_size;
	}
};

//0x7? 0111_xxyy -- x = *y
struct irLD : public Instruction
{
	irLD()
	{
		name = "LD";
		instruction_size = 1;
		num_parameters = 2;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int reg2)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}
		if (reg2 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg2 << "\".\n";
			return 0;
		}

		memory[address] = 0x70 + (reg1 << 2) + (reg2);
		++address;

		return instruction_size;
	}
};

//0x8? 1000_xxyy -- x += y
struct irAdd : public Instruction
{
	irAdd()
	{
		name = "ADD";
		instruction_size = 1;
		num_parameters = 2;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int reg2)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}
		if (reg2 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg2 << "\".\n";
			return 0;
		}

		memory[address] = 0x80 + (reg1 << 2) + (reg2);
		++address;

		return instruction_size;
	}
};

//0x9? 1001_xxyy -- x -= y
struct irSub : public Instruction
{
	irSub()
	{
		name = "SUB";
		instruction_size = 1;
		num_parameters = 2;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int reg2)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}
		if (reg2 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg2 << "\".\n";
			return 0;
		}

		memory[address] = 0x90 + (reg1 << 2) + (reg2);
		++address;

		return instruction_size;
	}
};

//0xA? 1010_xxyy -- x >>= y
struct irRShift : public Instruction
{
	irRShift()
	{
		name = "RSHIFT";
		instruction_size = 1;
		num_parameters = 2;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int reg2)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}
		if (reg2 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg2 << "\".\n";
			return 0;
		}

		memory[address] = 0xA0 + (reg1 << 2) + (reg2);
		++address;

		return instruction_size;
	}
};

//0xB? 1011_xx00 -- x = ~x
struct irNot : public Instruction
{
	irNot()
	{
		name = "NOT";
		instruction_size = 1;
		num_parameters = 1;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int = 0)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}

		memory[address] = 0xB0 + (4 * reg1);
		++address;

		return instruction_size;
	}
};

//0xB? 1011_xx01 -- PC = x
struct irJMP : public Instruction
{
	irJMP()
	{
		name = "JMP";
		instruction_size = 1;
		num_parameters = 1;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int = 0)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}

		memory[address] = 0xB1 + (4 * reg1);
		++address;

		return instruction_size;
	}
};

//0xB? 1011_xx10 -- PC = x iff C=1
struct irPCC : public Instruction
{
	irPCC()
	{
		name = "PCC";
		instruction_size = 1;
		num_parameters = 1;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int = 0)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}

		memory[address] = 0xB2 + (4 * reg1);
		++address;

		return instruction_size;
	}
};

//0xB? 1011_xx11 -- PC = x iff Z=1
struct irPCZ : public Instruction
{
	irPCZ()
	{
		name = "PCZ";
		instruction_size = 1;
		num_parameters = 1;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int = 0)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}

		memory[address] = 0xB3 + (4 * reg1);
		++address;

		return instruction_size;
	}
};

//0xC? 1100_xxyy -- x &= y
struct irAnd : public Instruction
{
	irAnd()
	{
		name = "AND";
		instruction_size = 1;
		num_parameters = 2;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int reg2)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}
		if (reg2 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg2 << "\".\n";
			return 0;
		}

		memory[address] = 0xC0 + (reg1 << 2) + (reg2);
		++address;

		return instruction_size;
	}
};

//0xD? 1101_xxyy -- x |= y
struct irOr : public Instruction
{
	irOr()
	{
		name = "OR";
		instruction_size = 1;
		num_parameters = 2;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int reg2)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}
		if (reg2 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg2 << "\".\n";
			return 0;
		}

		memory[address] = 0xD0 + (reg1 << 2) + (reg2);
		++address;

		return instruction_size;
	}
};

//0xE? 1110_xxyy -- x - y (no store)
struct irCMP : public Instruction
{
	irCMP()
	{
		name = "CMP";
		instruction_size = 1;
		num_parameters = 2;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int reg2)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}
		if (reg2 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg2 << "\".\n";
			return 0;
		}

		memory[address] = 0xE0 + (reg1 << 2) + (reg2);
		++address;

		return instruction_size;
	}
};

//0xF? 1111_xxyy -- *y = x
struct irRAM : public Instruction
{
	irRAM()
	{
		name = "RAM";
		instruction_size = 1;
		num_parameters = 2;
	}

	virtual int parse(uint8_t& address, uint8_t* memory, int reg1, int reg2)
	{
		//Validate instruction size.
		if (address >= RAM_SIZE - 1 - instruction_size)
		{
			std::cout << "Error: Program exceeded max size.\n";
			return 0;
		}

		//Validate registers.
		if (reg1 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg1 << "\".\n";
			return 0;
		}
		if (reg2 >= NUM_REGISTERS)
		{
			std::cout << "Error: Invalid register \"" << reg2 << "\".\n";
			return 0;
		}

		memory[address] = 0xF0 + (reg1 << 2) + (reg2);
		++address;

		return instruction_size;
	}
};

// ***** END INSTRUCTION IMPLEMENTATIONS *****





class InstructionParser
{
public:
	static const uint16_t NUM_INSTRUCTIONS = 1;

private:
	std::map<std::string, Instruction* > instructions;
	std::map<std::string, Instruction* >::iterator instructions_iter;

	//Array of labels keyed by their name, returns the memory address they point to.
	std::map<std::string, uint8_t> labels;
	std::map<std::string, uint8_t>::iterator labels_iter;

public:
	InstructionParser()
	{
		Instruction *instruction = new irNOP();
		instructions[instruction->name] = instruction;

		instruction = new irHalt();
		instructions[instruction->name] = instruction;

		instruction = new irSet();
		instructions[instruction->name] = instruction;

		instruction = new irPCL();
		instructions[instruction->name] = instruction;

		instruction = new irPCO();
		instructions[instruction->name] = instruction;

		instruction = new irPCS();
		instructions[instruction->name] = instruction;

		instruction = new irLDI();
		instructions[instruction->name] = instruction;

		instruction = new irLD();
		instructions[instruction->name] = instruction;

		instruction = new irAdd();
		instructions[instruction->name] = instruction;

		instruction = new irSub();
		instructions[instruction->name] = instruction;

		instruction = new irRShift();
		instructions[instruction->name] = instruction;

		instruction = new irNot();
		instructions[instruction->name] = instruction;

		instruction = new irJMP();
		instructions[instruction->name] = instruction;

		instruction = new irPCC();
		instructions[instruction->name] = instruction;

		instruction = new irPCZ();
		instructions[instruction->name] = instruction;

		instruction = new irAnd();
		instructions[instruction->name] = instruction;

		instruction = new irOr();
		instructions[instruction->name] = instruction;

		instruction = new irCMP();
		instructions[instruction->name] = instruction;

		instruction = new irRAM();
		instructions[instruction->name] = instruction;
	}

	~InstructionParser()
	{
		for (instructions_iter = instructions.begin(); instructions_iter != instructions.end(); ++instructions_iter)
		{
			delete (*instructions_iter).second;
			(*instructions_iter).second = nullptr;
		}

		instructions.clear();
	}

	bool isLabelDefinition(std::string symbol)
	{
		std::string::iterator i = symbol.end();

		if (i > symbol.begin() && *(i - 1) == ':') //Is a label definition
		{
			return true;
		}

		return false;
	}

	//Throws on error.
	void addLabel(std::string name, uint8_t memory_location)
	{
		if ((labels_iter = labels.find(name)) != labels.end())
		{
			std::cout << "Error: Redefinition of label \" " << name << "\"\n";
			//return false;
			throw;
		}

		*(name.end() - 1) = '\0'; //Get rid of the trailing colon.

		if ((instructions_iter = instructions.find(name)) != instructions.end() || name == "BYTE")
		{
			std::cout << "Error: Reserved keyword \"" << name << "\".\n";
			throw;
		}

		std::cout << "[DEBUG] Adding label: \"" << name << "\n";

		labels[name] = memory_location;
	}

	//Throws if does not find.
	uint8_t getLabel(std::string name)
	{
		if ((labels_iter = labels.find(name)) == labels.end())
		{
			std::cout << "Error: Undefined label \" " << name << "\"\n";
			throw;
		}

		return labels[name];
	}

	uint8_t regNameToNum(std::string name)
	{
		if (name.size() == 0)
		{
			return 0;
		}

		return name[0] - 'A'; //Only supports names A, B, C, D.
	}

	/*
	 * Parses for label definitions and inserts them into labels array.
	 * Also does some preliminary validation of program.
	 */
	void preprocess(std::vector<std::string>& source_code)
	{
		std::vector<std::string>::iterator source_counter;
		std::string source_symbol;

		uint8_t address = 0x00;

		for (source_counter = source_code.begin(); source_counter != source_code.end(); ++source_counter)
		{
			source_symbol = *source_counter;
			for (auto & i: source_symbol)
			{
				i = toupper(i);
			}

			if ((instructions_iter = instructions.find(source_symbol)) != instructions.end())
			{
				if (address + (*instructions_iter).second->instruction_size < address )
				{
					//Overflowed program memory, not enough space.
					std::cout << "Error: Program exceeds max size allowed on this architecture!\n";
					throw;
				}
				address += (*instructions_iter).second->instruction_size;
				source_counter += (*instructions_iter).second->num_parameters;
				continue; //Valid instruction. Move on.
			}

			if (source_symbol == "BYTE")
			{
				if (address + 1 < address)
				{
					//Overflowed program memory, not enough space.
					std::cout << "Error: Program exceeds max size allowed on this architecture!\n";
					throw;
				}
				++address;
				continue;
			}

			if (isLabelDefinition(source_symbol))
			{
				addLabel(source_symbol, address);
				source_code.erase(source_counter); //Don't process label definitions in the main assembling run.
				//Might be more efficient to swap this index & last, then pop off end.
				continue;
			}

			/*
			 * Final case: It's either a label or something invalid. Assume label and increment address by one byte.
			 */
			if (address + 1 < address)
			{
				//Overflowed program memory, not enough space.
				std::cout << "Error: Program exceeds max size allowed on this architecture!\n";
				throw;
			}
			++address;
		}
	}

	/*
	 * Paramaters:
	 * 		source_code		- input file split up into vector of individual words/symbols
	 * 		source_counter	- current location into source code
	 * 		address			- current address writing to in program memory.
	 * 		memory			- array that is the entire program memory.
	 */
	int parseInstruction(std::vector<std::string>& source_code, std::vector<std::string>::iterator& source_counter, uint8_t& address, uint8_t* memory)
	{
		//Don't care about capitalization.
		std::string source_symbol = *source_counter;
		for (auto & i: source_symbol)
		{
			i = toupper(i);
		}
		++source_counter;

		//Handle byte allocation
		if (source_symbol == "BYTE")
		{
			//Allocate a byte.
			uint8_t byte = std::stoi(*source_counter);
			memory[address] = byte;
			++source_counter;

			return 1;
		}

		//Handle instruction
		if ((instructions_iter = instructions.find(source_symbol)) == instructions.end())
		{
			try
			{
				//Handle labels. It's either a label or an invalid symbol.
				uint8_t label = getLabel(source_symbol);
				memory[address] = label;
				return 1; //Wrote out 1 byte.
			}
			catch(...)
			{
				std::cout << "Error: Invalid instruction \" " << source_symbol << "\"\n";
				throw;
			}
		}

		std::vector<uint8_t> parameters;
		for (uint8_t i = 0; i < (*instructions_iter).second->num_parameters; ++i)
		{
			if ((*source_counter).find_first_not_of( "0123456789" ) == std::string::npos)
			{
				//It's a number.
				parameters.push_back(std::stoi(*source_counter));
			}
			else
			{
				//Convert register name to number.
				parameters.push_back(regNameToNum(*source_counter));
			}
			++source_counter;
		}

		//Parse function requires two parameters. Generate NULL parameters if needed.
		for (uint8_t i = (*instructions_iter).second->num_parameters; i < 2; ++i)
		{
			parameters.push_back(0);
		}

		uint8_t bytes_written = (*instructions_iter).second->parse(address, memory, parameters[0], parameters[1]);
		if (bytes_written == 0)
		{
			//Error.
			throw;
		}
		return 0; //The instruction's parse function automatically increments this.
	}
};

class Program
{
public:
	InstructionParser &parser;

private:
	uint8_t memory[RAM_SIZE];

	std::vector<std::string> sourcecode;
	std::vector<std::string>::iterator source_pointer; //Current location in processing source code.

public:
	Program() :
		parser(*(new InstructionParser()))
	{
		uint16_t i = 0;
		for (i = 0; i < RAM_SIZE; ++i)
		{
			memory[i] = 0;
		}
	}

	void setByte(uint8_t byte, uint8_t value)
	{
		memory[byte] = value;
	}

	/*
	 * Load in file and pass off each instruction one by one to the instruction parser.
	 * Save output binary file that can be run in the emulator.
	 */
	bool assembleFile(std::string input, std::string output)
	{
		std::ifstream input_file(input);

		if (!input_file)
		{
			std::cout << "Error: Could not open input input_file \"" << input << "\"\n";
			return false;
		}

		//Break the file up into an array of words:std::string file_contents;

		std::string symbol;

		while (input_file >> symbol)
		{
			sourcecode.push_back(symbol);
			std::cout << "Read in: \"" << symbol << "\"\n";
			symbol = "";
		}

		input_file.close();

		//Preprocessor.
		try
		{
			parser.preprocess(sourcecode);
		}
		catch(...)
		{
			return false;
		}

		uint8_t address = 0x00;

		try
		{
			source_pointer = sourcecode.begin();
			while (source_pointer < sourcecode.end()) //Must increment source_pointer in parser.
			{
				address += parser.parseInstruction(sourcecode, source_pointer, address, memory);
			}
		}
		catch(...)
		{
			return false;
		}

		std::ofstream output_file(output, std::ios::binary);

		if (!output_file)
		{
			std::cout << "Error: Could not open input input_file \"" << output << "\"\n";
			return false;
		}

		output_file.write(reinterpret_cast<char* >(memory), RAM_SIZE);

		output_file.close();

		return true;
	}
};

void displayUsageInstructions(std::string default_input, std::string default_output)
{
	std::cout << "Program usage: \n" \
			<< "\n$> tas <input source file> <output binary file>\n\n" \
			<< "Default input: " << default_input \
			<< "\nDefault output: " << default_output << "\n";
}

int main(int argc, char **argv)
{
	/*
	 * Convert assembler mnemonics into binary opcodes for the CPU.
	 */

	std::string input_file = "program.tas";
	std::string output_file = "program.bin";

	//I'm going to set a hard limit on the command line arguments to 3 (2 actual useable arguments) for now.
	if (argc > 3)
	{
		displayUsageInstructions(input_file, output_file);
		return 1; //Blarg. They doin' it wrong.
	}

	if (argc >= 2)
	{
		if (!strcmp(argv[1], "-h"))
		{
			displayUsageInstructions(input_file, output_file);
			return 0;
		}
	}

	//Parse command line parameters.
	for (int i = 1; (i < argc) && (i < 3); ++i)
	{
		input_file = std::string(argv[i]);
	}

	Program program;


	if (!program.assembleFile(input_file, output_file))
	{
		return 1; //Failed to assemble.
	}

	return 0;
}
