#include <fstream>
#include <vector>

#include "DataTypes.h"
#include <array>
#include "Gpu.h"
#include <time.h>
#include <cstdlib>
#include <functional>
#include <SFML/Graphics.hpp>
#include <thread>

//typedef void (Cpu::*opCode8Function)(BYTE x, BYTE y);

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
	//returns BBCCDD
	WORD getLastThreeHalfBytes() const {
		return instruction & 0xFFF;
	}
};

class Cpu {
private:

	sf::RenderWindow m_window;
	bool m_close = false;

	std::array<BYTE, 16> V;
	WORD I = 0x0000;
	BYTE delay_timer = 0x00;
	BYTE sound_timer = 0x00;
	WORD pc = 0xC8;
	BYTE sp = 0x00;

	Instruction curr_instruction;

	/*std::array<opCode8Function, 9> opcode8functions = {
		&Cpu::_8xy0, &Cpu::_8xy1, &Cpu::_8xy2,
		&Cpu::_8xy3, &Cpu::_8xy4, &Cpu::_8xy5,
		&Cpu::_8xy6, &Cpu::_8xy7, &Cpu::_8xyE
	};*/

	std::array<BYTE, 64> m_stack;

	std::array<BYTE, 4096> m_ram;

	Gpu m_gpu;

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

		BYTE opCode = curr_instruction.getHalfByte1();

		switch (opCode)
		{
		case 0x0:
		{
			if (curr_instruction.getSecondByte() == 0xE0)
				_00E0();
			else if (curr_instruction.getSecondByte() == 0xEE)
				_00EE();
			else
				throw "Unknown OPCODE";
			break;
		}
		case 0x1:
		{
			_1nnn(curr_instruction.getLastThreeHalfBytes());
			break;
		}
		case 0x2:
		{
			_2nnn(curr_instruction.getLastThreeHalfBytes());
			break;
		}
		case 0x3:
		{
			_3xnn(curr_instruction.getHalfByte2(), curr_instruction.getSecondByte());
			break;
		}
		case 0x4:
		{
			_4xnn(curr_instruction.getHalfByte2(), curr_instruction.getSecondByte());
			break;
		}
		case 0x5:
		{
			_5xy0(curr_instruction.getHalfByte2(), curr_instruction.getHalfByte3());
			break;
		}
		case 0x6:
		{
			_6xnn(curr_instruction.getHalfByte2(), curr_instruction.getSecondByte());
			break;
		}
		case 0x7:
		{
			_7xnn(curr_instruction.getHalfByte2(), curr_instruction.getSecondByte());
			break;
		}
		case 0x8:
		{
			BYTE index = curr_instruction.getHalfByte4() == 0xE ? 8 : curr_instruction.getHalfByte4();
			/*if (index >= 9)throw "Unknown Opcode";
			std::invoke(opcode8functions[index],
				curr_instruction.getHalfByte2(), curr_instruction.getHalfByte3());*/
			break;
		}
		case 0x9:
		{
			_9xy0(curr_instruction.getHalfByte2(), curr_instruction.getHalfByte3());
			break;
		}
		case 0xA:
		{
			_Annn(curr_instruction.getLastThreeHalfBytes());
			break;
		}
		case 0xB:
		{
			_Bnnn(curr_instruction.getLastThreeHalfBytes());
			break;
		}
		case 0xC:
		{
			_Cxnn(curr_instruction.getHalfByte2(), curr_instruction.getSecondByte());
			break;
		}
		case 0xD:
		{
			_Dxyn(curr_instruction.getHalfByte2(), curr_instruction.getHalfByte3(),
				curr_instruction.getHalfByte4());
			break;
		}
		case 0xE:
		{
			if (curr_instruction.getSecondByte() == 0x9E)
			{
				_Ex9E(curr_instruction.getHalfByte2());
			}
			else if (curr_instruction.getSecondByte() == 0xA1)
			{
				_ExA1(curr_instruction.getHalfByte2());
			}
			else
				throw "Unknown OPCODE";

			break;
		}
		case 0xF:
		{
			//NOTE(sawii00): room for optimization if necessary
			BYTE secByte = curr_instruction.getSecondByte();
			if (secByte == 0x7) {
				_Fx07(curr_instruction.getHalfByte2());
			}
			else if (secByte == 0xA) {
				_Fx0A(curr_instruction.getHalfByte2());
			}
			else if (secByte == 0x15) {
				_Fx15(curr_instruction.getHalfByte2());
			}
			else if (secByte == 0x18) {
				_Fx18(curr_instruction.getHalfByte2());
			}
			else if (secByte == 0x1E) {
				_Fx1E(curr_instruction.getHalfByte2());
			}
			else if (secByte == 0x29) {
				_Fx29(curr_instruction.getHalfByte2());
			}
			else if (secByte == 0x33) {
				_Fx33(curr_instruction.getHalfByte2());
			}
			else if (secByte == 0x55) {
				_Fx55(curr_instruction.getHalfByte2());
			}
			else if (secByte == 0x65) {
				_Fx65(curr_instruction.getHalfByte2());
			}
			else
				throw "Unknown OPCODE";

			break;
		}
		default:
			throw "Unknown OPCODE";
		}
	}

public:

	Cpu() {
		reset();
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
		m_ram.fill(0);
		m_stack.fill(0);
		m_gpu.BufferReset();
		srand(time(nullptr));

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

		m_window.create(sf::VideoMode(1280, 640), "Chip8Emulator", sf::Style::None);
	}
	void start() {
		m_window.setActive(false);
		std::thread render_thread(Gpu::rendering, &m_window);
		// run the program as long as the window is open
		while (m_window.isOpen())
		{
			// check all the window's events that were triggered since the last iteration of the loop
			sf::Event event;
			while (m_window.pollEvent(event))
			{
				// "close requested" event: we close the window
				if (event.type == sf::Event::KeyPressed) {
					if (event.key.code == sf::Keyboard::Escape) {
						m_window.close();
						render_thread.join(); //to release thread and avoid terminate error
					}
				}
				if (m_close) {
					m_window.close();
					render_thread.join();
				}
			}
		}
	}
	void stop() {
		m_close = true;
	}

	void loadCartridge(const char* path) {
		std::ifstream cartridge(path, std::ios::binary);

		//0xDFF
		cartridge.read((char*)m_ram.data() + 200, 3544);
		cartridge.close();
	}

	//debugging
	void step();
	void dumpContext();
	void setBreakPoint(size_t address);

	//Instructions

	void _00E0() {
		//clear the screen
		m_gpu.BufferReset();
		pc++;
	}

	void _00EE() {
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

	void _Annn(WORD nnn) {
		I = nnn;
		pc++;
	}

	void _Bnnn(WORD nnn) {
		pc = nnn + V[0];
	}

	void _Cxnn(BYTE x, BYTE nn) {
		int r = rand() % 256;
		V[x] = r & nn;
		pc++;
	}

	void _Dxyn(BYTE x, BYTE y, BYTE n) {
		V[0xF] = m_gpu.DrawSprite(x, y, m_ram.data() + I, n);
		pc++;
	}

	void _Ex9E(BYTE x) {
	}

	void _ExA1(BYTE x) {
	}

	void _Fx07(BYTE x) {
		V[x] = delay_timer;
		pc++;
	}

	void _Fx0A(BYTE x) {
	}

	void _Fx15(BYTE x) {
		delay_timer = V[x];
		pc++;
	}

	void _Fx18(BYTE x) {
		sound_timer = V[x];
		pc++;
	}

	void _Fx1E(BYTE x) {
		I = V[x];
		pc++;
	}

	void _Fx29(BYTE x) {
		I = 5 * V[x];
		pc++;
	}

	void _Fx33(BYTE x) {
		BYTE h = x / 100;
		BYTE d = (x - h * 100) / 10;
		m_ram[I] = h;
		m_ram[I + 1] = d;
		m_ram[I + 2] = x - 100 * h - 10 * d;
		pc++;
	}

	void _Fx55(BYTE x) {
		for (int i = 0; i <= x; i++) {
			m_ram[I + i] = V[i];
		}

		I = I + x + 1;
		pc++;
	}

	void _Fx65(BYTE x) {
		for (int i = 0; i <= x; i++) {
			V[i] = m_ram[I + i];
		}
		I = I + x + 1;
		pc++;
	}
};
