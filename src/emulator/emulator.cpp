/* Copyright Ciprian Ilies 2016 */

#include <cstdint>
#include <iostream>
#include <fstream>

//Bitwise functions:
uint8_t setBit(uint8_t number, uint8_t bit, uint8_t value)
{
	return (number ^= (-value ^ number) & (1 << bit));
}

bool checkBit(uint8_t number, uint8_t bit)
{
	return ((number >> bit) & 1);
}

//CPU Components
class RegBank
{
public:
	static const uint8_t NUM_REGISTERS = 4;

private:
	uint8_t registers[NUM_REGISTERS];

public:
	RegBank()
	{
		for (uint8_t i = 0; i < NUM_REGISTERS; ++i)
		{
			registers[i] = 0x00;
		}
	}

	uint8_t getRegister(uint8_t x) const
	{
		if (x >= NUM_REGISTERS)
		{
			return 0;
		}

		return registers[x];
	}

	void setRegister(uint8_t x, uint8_t value)
	{
		if (x >= NUM_REGISTERS)
		{
			return;
		}

		registers[x] = value;
	}
};

class RAM
{
public:
	static const uint16_t RAM_SIZE = 256; //8-bit RAM.

private:
	uint8_t memory[RAM_SIZE];

public:
	RAM()
	{
		for (uint16_t i = 0; i < RAM_SIZE; ++i)
		{
			memory[i] = 0x00;
		}
	}

	//Returns ith byte in RAM.
	uint8_t getByte(uint8_t i) const
	{
		return memory[i];
	}

	void setByte(uint8_t i, uint8_t value)
	{
		memory[i] = value;
		std::cout << "Set byte " << static_cast<uint16_t>(i) << " to " << static_cast<uint16_t>(value) <<  "\n";
	}

	bool loadFromFileObject(std::ifstream &file)
	{
		if (!file)
		{
			return false;
		}

		std::streampos end;
		file.seekg(0, std::ios::end);
		end = file.tellg();
		if (end < RAM_SIZE)
		{
			std::cout << "Error: Input RAM file is too short!\n";
			return false;
		}

		if (end > RAM_SIZE)
		{
			std::cout << "Warning: RAM file is too big! Program may not function as you expect.\n";
		}

		file.seekg(0, std::ios::beg);

		if (!file.read(reinterpret_cast<char* >(memory), RAM_SIZE))
		{
			std::cout << "Error: Unknown error in reading RAM.\n";
			return false;
		}

		return true;
	}

	bool writeOutToFileObject(std::ofstream &file)
	{
		file.write(reinterpret_cast<char* >(memory), RAM_SIZE);

		return true;
	}
};

class ALU
{
	uint8_t flags; //Only first 5 bits are used.

public:
	ALU()
	{
		flags = 0x00;
	}

	/*
	 * Each flag parameter is input as a bit.
	 * Parameters:
	 * * c = carry flag
	 * * z = zero flag
	 * * s = sign flag
	 * * o = overflow flag
	 * * l = L flag
	 */
	void setFlags(uint8_t c, uint8_t z, uint8_t s, uint8_t o, uint8_t l)
	{
		flags = 0x00;
		flags = setBit(flags, 4, c);
		flags = setBit(flags, 3, z);
		flags = setBit(flags, 2, s);
		flags = setBit(flags, 1, o);
		flags = setBit(flags, 0, l);
	}

	bool getCFlag() const
	{
		return checkBit(flags, 4);
	}

	bool getZFlag() const
	{
		return checkBit(flags, 3);
	}

	bool getSFlag() const
	{
		return checkBit(flags, 2);
	}

	bool getOFlag() const
	{
		return checkBit(flags, 1);
	}

	bool getLFlag() const
	{
		return checkBit(flags, 0);
	}

	uint8_t add(uint8_t x, uint8_t y, bool cin)
	{
		uint8_t sum = x + y;

		//Flags
		bool c = (sum < x) ? 1 : 0;
		bool z = (sum == 0x00) ? 1 : 0;
		bool s = checkBit(sum, 7); //Most significant bit.
		bool s1 = checkBit(x, 7);
		bool s2 = checkBit(y, 7);
		bool o = ((!s1 && !s2 && s) || (s1 && s2 && !s)); //For addition: (!S1 && !S2 && !Sout) || (S1 && S2 && Sout )
		bool l = (s != o); //Sout XOR Oout

		setFlags(c, z, s, o, l);

		return sum;
	}

	uint8_t sub(uint8_t x, uint8_t y, bool cin)
	{
		uint8_t diff = x - y;

		//Flags
		bool c = (diff > x) ? 1 : 0; //I think this is right.
		bool z = (diff == 0x00) ? 1 : 0;
		bool s = checkBit(diff, 7); //Most significant bit.
		bool s1 = checkBit(x, 7);
		bool s2 = checkBit(y, 7);
		bool o = ((!s1 && s2 && s) || (s1 && !s2 && !s)); //For subtraction: ((!S1 && S2 && Sout) || (S1 && !S2 && !Sout))
		bool l = (s != o); //Sout XOR Oout

		setFlags(c, z, s, o, l);

		return diff;
	}

	uint8_t bitwiseNot(uint8_t x, bool cin)
	{
		x = ~x;

		//Flags
		bool c = 0;
		bool z = (x == 0x00) ? 1 : 0;
		bool s = checkBit(x, 7); //Most significant bit.
		bool o = 0;
		bool l = (s != o); //Sout XOR Oout

		setFlags(c, z, s, o, l);

		return x;
	}

	//TODO: Implement rest of operations.
};

class CPU
{
public:
	static const uint16_t NUM_OP_CODES = 256; //8-bit CPU.
	typedef void(CPU::*CPUFunctionPointer)(uint8_t, uint8_t);

	bool running;

private:
	RegBank &regbank;
	RAM &ram;
	ALU &alu;

	uint8_t program_counter; //Don't forget to increment after (almost) every instruction!
	uint8_t instruction;


	//CPU opcodes function pointers.
	//Could probably have used functors instead. Meh.

	//0x00 0000_0000 -- nop
	void opNop(uint8_t, uint8_t)
	{
		//std::cout << "No op.\n";

		++program_counter;
	}

	//0x01 0000_0001 -- halt
	void opHalt(uint8_t, uint8_t)
	{
		//std::cout << "[opHalt()] Halted.\n";

		running = false; //It's really that simple.
		//Do not increment program counter.
	}

	//0x5? 0101_xxyy -- x = y
	void opAssignDirect(uint8_t x, uint8_t y)
	{
		//std::cout << "[opAssignDirect()] Assign reg to reg (" << static_cast<uint16_t>(x) << ", " << static_cast<uint16_t>(y) << ")\n"; //Casting because uint8_t = char.

		regbank.setRegister(x, regbank.getRegister(y));
		++program_counter;
	}

	//0x6? 0110_00xx -- PC = x iff L=1
	void opPCL(uint8_t x, uint8_t y)
	{
		//std::cout << "[opPCL()] PC = value of register " << static_cast<uint16_t>(x) << " iff L = 1\n";

		if (alu.getLFlag())
		{
			program_counter = regbank.getRegister(x);
		}
		else
		{
			++program_counter;
		}
	}

	//0x6? 0110_01xx -- PC = x iff O=1
	void opPCO(uint8_t x, uint8_t y)
	{
		//std::cout << "[opPCO()] PC = value of register " << static_cast<uint16_t>(x) << " iff O = 1\n";

		if (alu.getOFlag())
		{
			program_counter = regbank.getRegister(x);
		}
		else
		{
			++program_counter;
		}
	}

	//0x6? 0110_10xx -- PC = x iff S=1
	void opPCS(uint8_t x, uint8_t y)
	{
		//std::cout << "[opPCS()] PC = value of register " << static_cast<uint16_t>(x) << " iff S = 1\n";

		if (alu.getSFlag())
		{
			program_counter = regbank.getRegister(x);
		}
		else
		{
			++program_counter;
		}
	}

	//0x6? 0110_11xx -- X = (*(PC++))
	void opLDI(uint8_t x, uint8_t y)
	{
		//std::cout << "[opLDI()] Register " << static_cast<uint16_t>(x) << " = LDI.\n";

		regbank.setRegister(x, ram.getByte(++program_counter));
		++program_counter;
	}

	//0x7? 0111_xxyy -- x = *y
	void opLD(uint8_t x, uint8_t y)
	{
		//std::cout << "[opLD()] Register " << static_cast<uint16_t>(x) << " = LD register " << static_cast<uint16_t>(y) << ".\n";

		regbank.setRegister(x, ram.getByte(regbank.getRegister(y)));
		++program_counter;
	}

	//0x8? 1000_xxyy -- x += y
	void opAdd(uint8_t x, uint8_t y)
	{
		//std::cout << "[opADD()] Register " << static_cast<uint16_t>(x) << " += register " << static_cast<uint16_t>(y) << ".\n";

		regbank.setRegister(x, alu.add(regbank.getRegister(x), regbank.getRegister(y), false));
		++program_counter;
	}

	//0x9? 1001_xxyy -- x -= y
	void opSub(uint8_t x, uint8_t y)
	{
		//std::cout << "[opSUB()] Register " << static_cast<uint16_t>(x) << " -= register " << static_cast<uint16_t>(y) << ".\n";

		regbank.setRegister(x, alu.sub(regbank.getRegister(x), regbank.getRegister(y), false));
		++program_counter;
	}

	//0xA? 1010_xxyy -- x >>= y
	void opRightShift(uint8_t x, uint8_t y)
	{
		//TODO: Implement.
		std::cout << "[opRightShift()] *** UNIMPLEMENTED OP CODE *** (0xA0 to 0xAF :: opRightShift())\n";

		++program_counter;
	}

	//0xB? 1011_xx00 -- x = ~x
	void opBitwiseNot(uint8_t x, uint8_t y)
	{
		//std::cout << "[opBitwiseNot()] Register " << static_cast<uint16_t>(x) << " = ~register " << static_cast<uint16_t>(x) << ".\n";

		regbank.setRegister(x, alu.bitwiseNot(regbank.getRegister(x), false));
		++program_counter;
	}

	//0xB? 1011_xx01 -- PC = x
	void opJMP(uint8_t x, uint8_t y)
	{
		//std::cout << "[opJMP()] PC = register " << static_cast<uint16_t>(x) << ".\n";

		program_counter = regbank.getRegister(x);
	}

	//0xB? 1011_xx10 -- PC = x iff C=1
	void opPCC(uint8_t x, uint8_t y)
	{
		//std::cout << "[opPCC()] PC = value of register " << static_cast<uint16_t>(x) << " iff C = 1\n";

		if (alu.getCFlag())
		{
			program_counter = regbank.getRegister(x);
		}
		else
		{
			++program_counter;
		}
	}

	//0xB? 1011_xx11 -- PC = x iff Z=1
	void opPCZ(uint8_t x, uint8_t y)
	{
		//std::cout << "opPCZ[()] PC = value of register " << static_cast<uint16_t>(x) << " iff Z = 1\n";

		if (alu.getZFlag())
		{
			program_counter = regbank.getRegister(x);
		}
		else
		{
			++program_counter;
		}
	}

	//0xC? 1100_xxyy -- x &= y
	void opBitwiseAnd(uint8_t x, uint8_t y)
	{
		//TODO: Implement.
		std::cout << "[opBitwiseAnd()] *** UNIMPLEMENTED OP CODE *** (0xC0 to 0xCF :: opBitwiseAnd())\n";

		++program_counter;
	}

	//0xD? 1101_xxyy -- x |= y
	void opBitwiseOr(uint8_t x, uint8_t y)
	{
		//TODO: Implement.
		std::cout << "[opBitwiseOr()] *** UNIMPLEMENTED OP CODE *** (0xD0 to 0xDF :: opBitwiseOr())\n";

		++program_counter;
	}

	//0xE? 1110_xxyy -- x - y (no store)
	void opCMP(uint8_t x, uint8_t y)
	{
		//std::cout << "[opCMP()] Register " << static_cast<uint16_t>(x) << " - register " << static_cast<uint16_t>(y) << " (no store).\n";

		alu.sub(regbank.getRegister(x), regbank.getRegister(y), false);
		++program_counter;
	}

	//0xF? 1111_xxyy -- *y = x
	void opSetRAM(uint8_t x, uint8_t y)
	{
		//std::cout << "[opSetRAM()] Ram pointed to by register " << static_cast<uint16_t>(y) << " = value of register " << static_cast<uint16_t>(x) << ".\n";

		ram.setByte(regbank.getRegister(y), regbank.getRegister(x));
		++program_counter;
	}

public:

	CPUFunctionPointer opcodes[NUM_OP_CODES];

	CPU() :
		regbank(*(new RegBank())),
		ram(*(new RAM())),
		alu(*(new ALU()))
	{
		running = true;

		program_counter = 0x00;
		instruction = 0x00;

		uint16_t i = 0;
		//Initialize blank opcodes.
		for (i = 0; i < NUM_OP_CODES; ++i)
		{
			opcodes[i] = &CPU::opHalt;
		}

		//Set opcodes.
		opcodes[0x00] = &CPU::opNop;
		//0x01 is already halt
		//0101xxyy: x = y
		for (i = 0x50; i <= 0x5F; ++i) //x = y
		{
			opcodes[i] = &CPU::opAssignDirect;
		}

		for (i = 0x60; i <= 0x63; ++i) //PC = x iff L = 1
		{
			opcodes[i] = &CPU::opPCL;
		}

		for (i = 0x64; i <= 0x67; ++i) //PC = x iff O = 1
		{
			opcodes[i] = &CPU::opPCO;
		}

		for (i = 0x68; i <= 0x6B; ++i) //PC = x iff S = 1
		{
			opcodes[i] = &CPU::opPCS;
		}

		for (i = 0x6C; i <= 0x6F; ++i) //x = (*(PC++))
		{
			opcodes[i] = &CPU::opLDI;
		}

		for (i = 0x70; i <= 0x7F; ++i) //x *= y
		{
			opcodes[i] = &CPU::opLD;
		}

		for (i = 0x80; i <= 0x8F; ++i) //x += y
		{
			opcodes[i] = &CPU::opAdd;
		}

		for (i = 0x90; i <= 0x9F; ++i) //x -= y
		{
			opcodes[i] = &CPU::opSub;
		}

		for (i = 0xA0; i <= 0xAF; ++i) //x >>= y
		{
			opcodes[i] = &CPU::opRightShift;//TODO: opRightShift()
		}

		for (i = 0xB0; i <= 0xBF; i += 4) //x ~= x
		{
			opcodes[i] = &CPU::opBitwiseNot;
		}

		for (i = 0xB1; i <= 0xBF; i += 4) //pc = x
		{
			opcodes[i] = &CPU::opJMP;
		}

		for (i = 0xB2; i <= 0xBF; i += 4) //PC = x iff C = 1
		{
			opcodes[i] = &CPU::opPCC;
		}

		for (i = 0xB3; i <= 0xBF; i += 4) //PC = x iff Z = 1
		{
			opcodes[i] = &CPU::opPCZ;
		}

		for (i = 0xC0; i <= 0xCF; ++i) //x &= y
		{
			opcodes[i] = &CPU::opBitwiseAnd; //TODO: opBitwiseAnd
		}

		for (i = 0xD0; i <= 0xDF; ++i) //x |= y
		{
			opcodes[i] = &CPU::opBitwiseOr; //TODO: opBitwiseOr
		}

		for (i = 0xE0; i <= 0xEF; ++i) //x - y
		{
			opcodes[i] = &CPU::opCMP;
		}

		for (i = 0xF0; i <= 0xFF; ++i) //x = *y
		{
			opcodes[i] = &CPU::opSetRAM;
		}
	}

	void executeInstruction(uint8_t opcode)
	{
		if (static_cast<uint16_t>(opcode) > NUM_OP_CODES || !running)
		{
			return;
		}

		std::cout << "Hex representation: 0x" << std::hex << static_cast<uint16_t>(opcode) << std::dec << " *** \n";

		//Specific cases:
		if (opcode >= 0x50 && opcode <= 0x5F) //Register x = y
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, opcode%4); //opAssignDirect()
		}
		else if (opcode >= 0x60 && opcode <= 0x63) //PC = x iff L = 1
		{
			(this->*opcodes[opcode])(opcode%4, 0x00); //opPCL()
		}
		else if (opcode >= 0x64 && opcode <= 0x67) //PC = x iff O = 1
		{
			(this->*opcodes[opcode])(opcode%4, 0x00); //opPCO()
		}
		else if (opcode >= 0x68 && opcode <= 0x6B) //PC = x iff S = 1
		{
			(this->*opcodes[opcode])(opcode%4, 0x00);//opPCS()
		}
		else if (opcode >= 0x6C && opcode <= 0x6F) //x = (*(PC++))
		{
			(this->*opcodes[opcode])(opcode%4, 0x00); //opLDI()
		}
		else if (opcode >= 0x70 && opcode <= 0x7F) //x = *y
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, opcode%4); //opLD()
		}
		else if (opcode >= 0x80 && opcode <= 0x8F) //x += y
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, opcode%4); //opAdd()
		}
		else if (opcode >= 0x90 && opcode <= 0x9F) //x -= y
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, opcode%4); //opSub()
		}
		else if (opcode >= 0xA0 && opcode <= 0xAF) //x >>= y
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, opcode%4); //TODO opRightShift()
		}
		else if (opcode >= 0xB0 && opcode <= 0xBF && (opcode%4 == 0)) //x ~= x
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, 0); //opBitwiseNot()
		}
		else if (opcode >= 0xB0 && opcode <= 0xBF && (opcode%4 == 1)) //PC = x
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, 0); //opJMP()
		}
		else if (opcode >= 0xB0 && opcode <= 0xBF && (opcode%4 == 2)) //PC = x iff C = 1
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, 0); //opPCC()
		}
		else if (opcode >= 0xB0 && opcode <= 0xBF && (opcode%4 == 3)) //PC = x iff Z = 1
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, 0); //opPCZ()
		}
		else if (opcode >= 0xC0 && opcode <= 0xCF) //x &= y
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, opcode%4); //TODO: opBitwiseAnd()
		}
		else if (opcode >= 0xD0 && opcode <= 0xDF) //x |= y
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, opcode%4); //TODO: opBitwiseOr()
		}
		else if (opcode >= 0xE0 && opcode <= 0xEF) //x - y
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, opcode%4); //opCMP()
		}
		else if (opcode >= 0xF0 && opcode <= 0xFF) //*y = x
		{
			(this->*opcodes[opcode])((opcode >> 2)%4, opcode%4); //opSetRAM()
		}
		//General case:
		else
		{
			(this->*opcodes[opcode])(0x00, 0x00);
		}
	}

	bool validateProgram()
	{
		//All it does right now is check to make sure you don't have an empty program (only no-ops).

		for (uint16_t i = 0; i < ram.RAM_SIZE; ++i)
		{
			if (ram.getByte(i))
			{
				return true;
			}
		}

		std::cout << "Warning: Program has no instructions! Just an empty infinite loop, not running this program.\n";
		return false;
	}

	//Loads the input program (actually entire RAM file).
	bool loadRAM(std::string file)
	{
		std::ifstream f(file, std::ios::binary);

		if (!f)
		{
			std::cout << "Error: failed to open file for input program/RAM: \"" << file << "\"\n";
			return false;
		}

		ram.loadFromFileObject(f);

		f.close();

		if (!validateProgram())
		{
			return false;
		}

		return true;
	}

	bool writeOutRAM(std::string file)
	{
		std::ofstream f(file, std::ios::binary);

		if (!f)
		{
			std::cout << "Error: failed to open file for outputting final state of RAM: \"" << file << "\"\n";
			return false;
		}

		ram.writeOutToFileObject(f);

		f.close();

		return true;
	}

	void run()
	{
		uint64_t count = 0;
		while (running)
		{
			instruction = ram.getByte(program_counter);
			executeInstruction(instruction);

			++count;
		}

		std::cout << "\n\nExecuted " << count << " instructions.\n\n";
	}
};

int main()
{
	/*
	 * Basically:
	 * * Run assembler on your assembly program (or use a hex editor and create a 256 byte file).
	 * * Run emulator with your program fed into it.
	 * Note that program will abort given a RAM file that is too small (needs to be at least 256 bytes.
	 * Note that anything over 256 bytes is not loaded.
	 * Also, if your program has no actual instructions (just a file full of only no-ops (0x00)),
	    this emulator will abort (because that'd be a rather dull infinite loop).
	 */

	/*
	 * Instruction implementation:
	 * * Only vaguely simulates the CPU components.
	 * * Just reads in a bunch of uint8_ts and uses them to index into an array of function pointers that execute the opcode
	    that instruction represents.
	 */

	std::string input_file = "program.bin";
	std::string output_file = "output.bin";

	CPU cpu;

	if (!cpu.loadRAM(input_file))
	{
		return 0;
	}

	cpu.run();

	//Save final program state.
	cpu.writeOutRAM(output_file);

	return 0;
}