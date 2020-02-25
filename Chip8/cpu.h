#include <fstream>
#include "DataTypes.h"
#include <array>
#include "Gpu.h"
#include <time.h>
#include <cstdlib>
#include <functional>
#include <SFML/Graphics.hpp>
#include <thread>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Audio.hpp>
#include <chrono>

#ifdef CHIP8_DEBUG
#include <vector>
#define DEBUG 1
#else
#define DEBUG 0
#endif

class Cpu;

typedef void (Cpu::*opCode8Function)(BYTE x, BYTE y);

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
	#if DEBUG
	sf::RenderWindow m_debug_window;
	std::vector<DWORD> exec_time;

	#endif

	std::array<BYTE, 16> V;
	WORD I = 0x0000;
	BYTE delay_timer = 0x00;
	BYTE sound_timer = 0x00;
	WORD pc = 0x200;
	BYTE sp = 0x00;

	sf::SoundBuffer m_sound_buffer;
	sf::Sound m_audio_clip;

	Instruction curr_instruction;

	std::array<opCode8Function, 9> opcode8functions = {
		&Cpu::_8xy0, &Cpu::_8xy1, &Cpu::_8xy2,
		&Cpu::_8xy3, &Cpu::_8xy4, &Cpu::_8xy5,
		&Cpu::_8xy6, &Cpu::_8xy7, &Cpu::_8xyE
	};

	std::array<WORD, 64> m_stack;

	std::array<BYTE, 4096> m_ram;

	Gpu m_gpu;

	BYTE readByteFromRam(WORD address) {
		return m_ram[address];
	}
	WORD readWordFromRam(WORD address) {
		return (m_ram[address++] << 8 | m_ram[address]);
	}

	BYTE convertKeyCode(BYTE code) const {
		if (code >= 0 && code <= 9)
			return code + 26;
		else if (code >= 0xA && code <= 0xF)
			return code - 10;
		else
			return 100;
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
			if (index >= 9)throw "Unknown Opcode";
			//std::invoke(*opcode8functions[index], curr_instruction.getHalfByte2(), curr_instruction.getHalfByte3());
			(this->*opcode8functions[index])(curr_instruction.getHalfByte2(), curr_instruction.getHalfByte3());
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

	BYTE keyPressed() const {
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num0)) { return 0; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) { return 1; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) { return 2; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) { return 3; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) { return 4; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num5)) { return 5; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num6)) { return 6; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num7)) { return 7; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num8)) { return 8; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num9)) { return 9; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { return 0xA; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::B)) { return 0xB; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::C)) { return 0xC; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) { return 0xD; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) { return 0xE; }
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) { return 0xF; }
		else { return 0xFF; }
	}
	#if DEBUG

	static void debugging(sf::RenderWindow* window, Cpu* cpu) {
		window->setActive(true);
		sf::Text i_reg;
		sf::Font font;
		font.loadFromFile("Arial.ttf");
		i_reg.setFont(font);
		i_reg.setCharacterSize(15);
		i_reg.setPosition(sf::Vector2f(0, 0));

		sf::Text delay;
		delay.setFont(font);
		delay.setCharacterSize(15);
		delay.setPosition(sf::Vector2f(0, 30));

		sf::Text sound;
		sound.setCharacterSize(15);
		sound.setFont(font);
		sound.setPosition(sf::Vector2f(0, 60));

		sf::Text v_j;
		v_j.setCharacterSize(15);
		v_j.setFont(font);

		sf::Text p_c;
		p_c.setFont(font);
		p_c.setPosition(sf::Vector2f(0, 90));
		p_c.setCharacterSize(15);

		sf::Text ex_time;
		ex_time.setFont(font);
		ex_time.setCharacterSize(10);

		while (window->isOpen()) {
			window->clear(sf::Color::Black);

			i_reg.setString("Register I " + std::to_string(cpu->I));

			delay.setString("Delay timer " + std::to_string(cpu->delay_timer));

			sound.setString("Sound timer " + std::to_string(cpu->sound_timer));

			for (int j = 0; j < 16; j++) {
				v_j.setString("V" + std::to_string(j) + "\t\t" + std::to_string(cpu->V[j]));
				v_j.setPosition(sf::Vector2f(150, 20 * j));
				window->draw(v_j);
			}

			int count = 0;
			for (int j = cpu->exec_time.size() - 1; j >= cpu->exec_time.size() - 20; j--) {
				//ex_time.setString(std::to_string(cpu->exec_time[j]));
				ex_time.setPosition(250, 20 * count);
				window->draw(ex_time);
				count = count < 20 ? count + 1 : 0;
			}

			window->draw(i_reg);
			window->draw(delay);
			window->draw(sound);
			window->draw(p_c);
			window->display();
		}
	}
	#endif

public:

	Cpu() {
		reset();

		loadCartridge("Tank.ch8");
	}

	void reset() {
		for (int i = 0; i < 16; i++)
		{
			V[i] = 0;
		}
		I = 0;
		delay_timer = 0;
		sound_timer = 0;
		pc = 0x200;
		sp = 0;
		m_ram.fill(0);
		m_stack.fill(0);
		m_gpu.BufferReset();
		srand(time(nullptr));

		m_sound_buffer.loadFromFile("audio.wav");
		m_audio_clip.setBuffer(m_sound_buffer);
		m_audio_clip.setLoop(true);
		m_audio_clip.setVolume(20);

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
		if (m_window.isOpen())
			m_window.close();
		m_window.create(sf::VideoMode(1280, 640), "Chip8Emulator", sf::Style::None);
		#if DEBUG
		if (m_debug_window.isOpen())
			m_debug_window.close();
		m_debug_window.create(sf::VideoMode(400, 400), "Chip8EmulatorDebugger", sf::Style::Default);
		#endif
	}
	void start() {
		m_window.setActive(false);
		std::thread render_thread(Gpu::rendering, &m_window);
		#if DEBUG
		m_debug_window.setActive(false);
		std::thread debug_thread(Cpu::debugging, &m_debug_window, this);

		#endif
		// run the program as long as the window is open
		sf::Event event;
		sf::Clock clock;  //starts clock counting
		DWORD accumulator = 0;

		while (m_window.isOpen())
		{
			clock.restart();

			// check all the window's events that were triggered since the last iteration of the loop
			while (m_window.pollEvent(event))
			{
				// "close requested" event: we close the window
				if (event.type == sf::Event::KeyPressed) {
					if (event.key.code == sf::Keyboard::Escape) {
						m_window.close();
						render_thread.join(); //to release thread and avoid terminate error
						#if DEBUG
						m_debug_window.close();
						debug_thread.join();
						#endif
					}
				}
			}

			execute(0);
			sf::Time finish_time = clock.getElapsedTime();  //finishes clock counting

			/*
			accumulator += finish_time.asMicroseconds(); //gets the time in milliseconds
			//exec_time.push_back(finish_time.asMicroseconds());

			while (accumulator >= 17000) {
				if (delay_timer > 0)
					delay_timer--;
				if (sound_timer > 0) {
					sound_timer--;
					if (m_audio_clip.getStatus() != sf::SoundSource::Playing)
						m_audio_clip.play();
				}
				else {
					if (m_audio_clip.getStatus() != sf::SoundSource::Stopped)
						m_audio_clip.stop();
				}

				accumulator -= 17000;
			}*/
		}
	}
	void stop() {
		//does nothing
	}

	void loadCartridge(const char* path) {
		std::ifstream cartridge(path, std::ios::binary);

		//0xDFF
		cartridge.read((char*)m_ram.data() + 0x200, 3544);
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
		pc += 2;
	}

	void _00EE() {
		//returns from subroutine
		if (sp) {
			pc = m_stack[--sp] + 2;
		}
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
		if (V[x] == nn)pc += 4;
		else pc += 2;
	}
	void _4xnn(BYTE x, BYTE nn) {
		if (V[x] != nn)pc += 4;
		else pc += 2;
	}
	void _5xy0(BYTE x, BYTE y) {
		if (V[x] == V[y])pc += 4;
		else pc += 2;
	}

	void _6xnn(BYTE x, BYTE nn) {
		V[x] = nn;
		pc += 2;
	}
	void _7xnn(BYTE x, BYTE nn) {
		V[x] += nn;
		pc += 2;
	}

	void _8xy0(BYTE x, BYTE y) {
		V[x] = V[y];
		pc += 2;
	}
	void _8xy1(BYTE x, BYTE y) {
		V[x] |= V[y];
		pc += 2;
	}
	void _8xy2(BYTE x, BYTE y) {
		V[x] &= V[y];
		pc += 2;
	}
	void _8xy3(BYTE x, BYTE y) {
		V[x] ^= V[y];
		pc += 2;
	}
	void _8xy4(BYTE x, BYTE y) {
		WORD res = V[x] + V[y];
		V[0xF] = res > 255 ? 1 : 0;
		V[x] = res & 0xFF;
		pc += 2;
	}
	void _8xy5(BYTE x, BYTE y) {
		BYTE res = V[x] - V[y];
		V[0xF] = res < 0 ? 0 : 1;
		V[x] = res;
		pc += 2;
	}
	void _8xy6(BYTE x, BYTE y) {
		V[0xF] = V[y] & 0x1; // least significant bit
		V[x] = V[y] >> 0x1;
		pc += 2;
	}
	void _8xy7(BYTE x, BYTE y) {
		BYTE res = V[y] - V[x];
		V[0xF] = res < 0 ? 0 : 1;
		V[x] = res;
		pc += 2;
	}
	void _8xyE(BYTE x, BYTE y) {
		V[0xF] = V[y] & 0x80; //most significant bit
		V[x] = V[y] << 0x1;
		pc += 2;
	}

	void _9xy0(BYTE x, BYTE y) {
		if (V[x] != V[y])pc += 2;
		else pc += 2;
	}

	void _Annn(WORD nnn) {
		I = nnn;
		pc += 2;
	}

	void _Bnnn(WORD nnn) {
		pc = nnn + V[0];
	}

	void _Cxnn(BYTE x, BYTE nn) {
		int r = rand() % 256;
		V[x] = r & nn;
		pc += 2;
	}

	void _Dxyn(BYTE x, BYTE y, BYTE n) {
		V[0xF] = m_gpu.DrawSprite(V[x], V[y], m_ram.data() + I, n);
		pc += 2;
	}

	void _Ex9E(BYTE x) {
		if (sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>((V[x])))) {
			pc += 4;
		}
	}

	void _ExA1(BYTE x) {
		if (!sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>((V[x])))) {
			pc += 4;
		}
	}

	void _Fx07(BYTE x) {
		V[x] = delay_timer;
		pc += 2;
	}

	void _Fx0A(BYTE x) {
		BYTE key = keyPressed();
		if (key != 0xFF) {
			V[x] = key;
			pc += 2;
		}
	}

	void _Fx15(BYTE x) {
		delay_timer = V[x];
		pc += 2;
	}

	void _Fx18(BYTE x) {
		sound_timer = V[x];
		pc += 2;
	}

	void _Fx1E(BYTE x) {
		I += V[x];
		pc += 2;
	}

	void _Fx29(BYTE x) {
		I = 5 * V[x];
		pc += 2;
	}

	void _Fx33(BYTE x) {
		BYTE val = V[x];
		BYTE h = val / 100;
		BYTE d = (val - h * 100) / 10;
		m_ram[I] = h;
		m_ram[I + 1] = d;
		m_ram[I + 2] = val - 100 * h - 10 * d;
		pc += 2;
	}

	void _Fx55(BYTE x) {
		for (int i = 0; i <= x; i++) {
			m_ram[I + i] = V[i];
		}

		I = I + x + 1;
		pc += 2;
	}

	void _Fx65(BYTE x) {
		for (int i = 0; i <= x; i++) {
			V[i] = m_ram[I + i];
		}
		I = I + x + 1;
		pc += 2;
	}
};
