#include <vector>
#include "DataTypes.h"
#include <array>

struct Instruction {
	WORD instruction;
	// AABB|CCDD

	Instruction(BYTE first, BYTE second) {
		instruction = (first << 8) | second;
	}
	Instruction(WORD i = 0) :instruction(i) {}
	//returns AA
	BYTE getHalfByte1() const {
		return (instruction >> 12) & 0xF;
	}
	//returns BB
	BYTE getHalfByte2() const {
		return (instruction >> 8) & 0xF;
	}
	//return CC
	BYTE getHalfByte3() const {
		return (instruction >> 4) & 0xF;
	}
	//return DD
	BYTE getHalfByte4() const {
		return (instruction & 0xF);
	}
	//returns AABB
	BYTE getFirstByte() const {
		return (instruction >> 8) & 0xFF;
	}
	//returns CCDD
	BYTE getSecondByte() const {
		return instruction & 0xFF;
	}
};

class Cpu {
private:

	std::array<BYTE, 16> V;
	WORD I = 0x0000;
	BYTE delay_timer = 0x00;
	BYTE sound_timer = 0x00;
	WORD pc = 0xC8;
	BYTE sp = 0x00;

	Instruction curr_instruction;

	std::array<BYTE, 64> m_stack;

	std::array<BYTE, 4096> m_ram;

	//@TODO(sawii): test the reading from memory

	BYTE readByteFromRam(WORD address) {
		return m_ram[address];
	}
	WORD readWordFromRam(WORD address) {
		return (m_ram[address++] << 8 & m_ram[address]);
	}

	//executes the fetched instructions
	void execute(size_t n_steps) {
		curr_instruction = Instruction(readWordFromRam(pc));
	}

public:

	Cpu() {
		reset();

		BYTE sprites[]{
			0xF0, 0x90, 0x90, 0x90, 0xF0, //0
			0x20, 0x60, 0x20, 0x20, 0x70, //1
			0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
			0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
			0x90, 0x90, 0xF0, 0x10, 0x10, //4
			0xF0, 0x80, 0xF0, 0x10, 0xF0,  //5
			0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
			0xF0, 0x10, 0x20, 0x40, 0x40, //7
			0xF0, 0x90, 0xF0, 0x90, 0xF0,  //8
			0xF0, 0x90, 0xF0, 0x10, 0xF0,  //9
			0xF0, 0x90, 0xF0, 0x90, 0x90, //A
			0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
			0xF0, 0x80, 0x80, 0x80, 0xF0, //C
			0xE0, 0x90, 0x90, 0x90, 0xE0, //D
			0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
			0xF0, 0x80, 0xF0, 0x80, 0x80 //F
		};

		for (int i = 0; i < 80; i++) {
			m_ram[i] = sprites[i];
		}
	}

	void reset() {
		for (int i = 0; i < 16; i++)
		{
			V[i] = 0;
		}
		I = 0;
		delay_timer = 0;
		sound_timer = 0;
		pc = 0xC8;
		sp = 0;
		for (int i = 0; i < 64; i++)
		{
			m_stack[i] = 0;
		}
		for (int i = 0; i < 4096; i++)
		{
			m_ram[i] = 0;
		}
	}
	void start();
	void stop();

	void loadCartridge(const char* path) {
		FILE *cartridge = fopen(path, "rb");
		//0xDFF
		fread(&m_ram[0x200], 1, 3544, cartridge);
		fclose(cartridge);
	}

	//debugging
	void step();
	void dumpContext();
	void setBreakPoint(size_t address);

	//Instructions

	void _00E0() {
		//clear the screen
		pc++;
	}

	void _00ET() {
		//returns from subroutine
		pc = m_stack[sp];
		sp = !sp ? sp - 1 : 0; //if sp was 0 (should not occur) remains 0, else it decrements
	}
	void _1nnn(WORD address) {
		//jumps to address
		pc = address;
	}
	void _2nnn(WORD address) {
		//calls subroutine at address
		m_stack[sp++] = pc;
		pc = address;
	}
	void _3xnn(BYTE x, BYTE nn) {
		if (V[x] == nn)pc += 2;
		else pc++;
	}
	void _4xnn(BYTE x, BYTE nn) {
		if (V[x] != nn)pc += 2;
		else pc++;
	}
	void _5xy0(BYTE x, BYTE y) {
		if (V[x] == V[y])pc += 2;
		else pc++;
	}

	void _6xnn(BYTE x, BYTE nn) {
		V[x] = nn;
		pc++;
	}
	void _7xnn(BYTE x, BYTE nn) {
		V[x] += nn;
		pc++;
	}

	void _8xy0(BYTE x, BYTE y) {
		V[x] = V[y];
		pc++;
	}
	void _8xy1(BYTE x, BYTE y) {
		V[x] |= V[y];
		pc++;
	}
	void _8xy2(BYTE x, BYTE y) {
		V[x] &= V[y];
		pc++;
	}
	void _8xy3(BYTE x, BYTE y) {
		V[x] ^= V[y];
		pc++;
	}
	void _8xy4(BYTE x, BYTE y) {
		WORD res = V[x] + V[y];
		V[0xF] = res > 255 ? 1 : 0;
		V[x] = res & 0xFF;
		pc++;
	}
	void _8xy5(BYTE x, BYTE y) {
		BYTE res = V[x] - V[y];
		V[0xF] = res < 0 ? 0 : 1;
		V[x] = res;
		pc++;
	}
	void _8xy6(BYTE x, BYTE y) {
		V[0xF] = V[y] & 0x1; // least significant bit
		V[x] = V[y] >> 0x1;
		pc++;
	}
	void _8xy7(BYTE x, BYTE y) {
		BYTE res = V[y] - V[x];
		V[0xF] = res < 0 ? 0 : 1;
		V[x] = res;
		pc++;
	}
	void _8xyE(BYTE x, BYTE y) {
		V[0xF] = V[y] & 0x80; //most significant bit
		V[x] = V[y] << 0x1;
		pc++;
	}

	void _9xy0(BYTE x, BYTE y) {
		if (V[x] != V[y])pc += 2;
		else pc++;
	}
};
