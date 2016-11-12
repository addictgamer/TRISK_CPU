/* Copyright Ciprian Ilies 2016 */

#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

void displayUsageInstructions(std::string default_input, std::string default_output)
{
	std::cout << "Program usage: \n" \
			<< "\n$> bin2logisim <input program file> <output RAM file>\n\n" \
			<< "Default input: " << default_input \
			<< "\nDefault output: " << default_output << "\n";
}

static const uint16_t RAM_SIZE = 256; //8-bit RAM.

int main(int argc, char **argv)
{
	std::string input_filename = "program.bin";
	std::string output_filename = "processor.ram";

	//I'm going to set a hard limit on the command line arguments to 3 (2 actual useable arguments) for now.
	if (argc > 3)
	{
		displayUsageInstructions(input_filename, output_filename);
		return 1; //Blarg. They doin' it wrong.
	}

	if (argc >= 2)
	{
		if (!strcmp(argv[1], "-h"))
		{
			displayUsageInstructions(input_filename, output_filename);
			return 0;
		}
	}

	//Parse command line parameters.
	for (int i = 1; (i < argc) && (i < 3); ++i)
	{
		if (i == 1)
		{
			input_filename = std::string(argv[i]);
		}
		else if (i == 2)
		{
			output_filename = std::string(argv[i]);
		}
	}

	std::ifstream input_file(input_filename, std::ios::binary);

	if (!input_file)
	{
		std::cout << "Error: failed to open file for input program: \"" << input_filename << "\"\n";
		return 1;
	}

	std::streampos end;
	input_file.seekg(0, std::ios::end);
	end = input_file.tellg();
	if (end < RAM_SIZE)
	{
		std::cout << "Error: Input RAM file is too short!\n";
		input_file.close();
		return 1;
	}

	if (end > RAM_SIZE)
	{
		std::cout << "Warning: RAM file is too big! Program may not function as you expect.\n";
	}

	input_file.seekg(0, std::ios::beg);

	//Now, actually read in file.
	uint8_t program_memory[RAM_SIZE];

	if (!input_file.read(reinterpret_cast<char* >(program_memory), RAM_SIZE))
	{
		std::cout << "Error: Unknown error in reading in RAM file.\n";
		input_file.close();
		return 1;
	}

	input_file.close();

	std::ofstream output_file(output_filename);

	if (!output_file)
	{
		std::cout << "Error: failed to open file for output: \"" << output_filename << "\"\n";
		return false;
	}

	//Write out header.
	output_file << "v2.0 raw\n";

	for (uint16_t i = 0; i < RAM_SIZE; ++i)
	{
		output_file << std::hex << static_cast<uint16_t>(program_memory[i]) << " ";
	}

	output_file.close();

	return 0;
}
