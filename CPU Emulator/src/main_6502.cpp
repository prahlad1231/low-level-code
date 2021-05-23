#include <stdio.h>
#include <stdlib.h>

using Byte = unsigned char;
using Word = unsigned short;

using u32 = unsigned int;

struct Mem {
	static constexpr u32 MAX_MEM = 1024 * 64; // evaluate the value at compile time, C++ 11
	Byte data[MAX_MEM];

	void initialize() {
		for (u32 i = 0; i < MAX_MEM; ++i) {
			data[i] = 0;
		}
	}

	// read 1 byte
	Byte operator[] (u32 address) const {
		// assert here address < MAX_MEM
		return data[address];
	}

	// write 1 byte
	Byte& operator[] (u32 address) {
		return data[address];
	}

	// write 1 word i.e. 2 bytes
	void writeWord(Word word, u32 address, u32 cycles) {
		data[address] = word & 0xFF;
		data[address + 1] = (word >> 8);
		cycles -= 2;
	}
};

struct CPU {
	Word PC; // program counter
	Word SP; // stack pointer 

	Byte A, X, Y; // registers where A = Accumulator, X,Y = Index Registers

	// status flags where only one bit is allowed 
	Byte C : 1; // carry flag
	Byte Z : 1; // zero flag
	Byte I : 1; // interrupt flag
	Byte D : 1; // decimal flag
	Byte B : 1; // break flag
	Byte V : 1; // overflow flag
	Byte N : 1; // negative flag

	// according to commodore 64's reset routine with few modifications
	void reset(Mem& memory) {
		PC = 0xFFFC;
		SP = 0x0100;
		D = 0;
		A = X = Y = 0;
		C = Z = I = D = B = V = N = 0; // resetting all the flags instead of just decimal flag, for simplicity
		memory.initialize();
	}

	Byte fetchByte(u32& cycles, Mem& memory) {
		Byte data = memory[PC];
		PC++;
		cycles--;
		return data;
	}

	Word fetchWord(u32& cycles, Mem& memory) {
		// 6502 is little endian, so is this test system, incase you want to handle endianness, 
		// you'd have to swap bytes of data
		Word data = memory[PC];
		PC++;

		data |= (memory[PC] << 8);
		PC++;

		cycles += 2;

		return data;
	}

	Byte readByte(u32& cycles, Byte address, Mem& memory) {
		Byte data = memory[address];
		cycles--;
		return data;
	}

	// opcodes
	static constexpr Byte
		INS_LDA_IM = 0xA9, // load accumulator in immediate mode (addressing mode) (commodore 64)
		INS_LDA_ZP = 0xA5, // zero page addressing mode
		INS_LDA_ZPX = 0xB5, // zero page X addressing mode
		INS_JSR = 0x20; // jump sub routine, absolute addressing

	void LDASetStatus() {
		Z = (A == 0);
		N = (A & 0b10000000) > 0; // if bit 7 of A is set then set negative flag (commodore 64)
	}

	void execute(u32 cycles, Mem& memory) { // cycles = number of cpu cycles
		while (cycles > 0) {
			Byte ins = fetchByte(cycles, memory);
			switch (ins) {
			case INS_LDA_IM: {
				Byte value = fetchByte(cycles, memory);
				A = value;
				LDASetStatus();
			} break;
			case INS_LDA_ZP: {
				Byte zeroPageAddress = fetchByte(cycles, memory);
				A = readByte(cycles, zeroPageAddress, memory);
				LDASetStatus();
			} break;
			case INS_LDA_ZPX: {
				Byte zeroPageAddress = fetchByte(cycles, memory);
				zeroPageAddress += X;
				cycles--;
				A = readByte(cycles, zeroPageAddress, memory);
			} break;
			case INS_JSR: {
				Word address = fetchWord(cycles, memory);
				memory.writeWord(PC - 1, SP, cycles);
				PC = address;
				cycles--;
			} break;
			default:
				printf("Instruction not handled %d", ins);
				break;
			}
		}
	}
};

int main() {
	Mem mem;
	CPU cpu;
	cpu.reset(mem);
	printf("Starting program...\n");
	// test program
	/*mem[0xFFFC] = CPU::INS_LDA_IM;
	mem[0xFFFD] = 0x42;
	mem[0x0042] = 0x84;
	
	cpu.execute(3, mem);*/

	// testing JSR and loading data in register
	mem[0xFFFC] = CPU::INS_JSR;
	mem[0xFFFD] = 0x42;
	mem[0xFFFE] = 0x42;
	mem[0x4242] = CPU::INS_LDA_IM;
	mem[0x4243] = 0x64; // loading 100d in A

	cpu.execute(9, mem);
	return 0;
}
