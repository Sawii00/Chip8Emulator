#pragma once

#include "DataTypes.h"
#include <array>

class Gpu {
private:

	std::array<BYTE, 64 * 32> frame_buffer;

public:

	Gpu() {
		BufferReset();
	}

	bool GetPixel(BYTE x, BYTE y) {
		return frame_buffer[y * 64 + x];
	}

	//reset frame buffer
	void BufferReset() {
		frame_buffer.fill(0);
	}

	//settare pixel generico
	void PixelSet(BYTE x, BYTE y, bool choice) {
		frame_buffer[y * 64 + x] = choice;
	}

	//ingresso un array di byte e la sua dim.
	bool DrawSprite(BYTE x, BYTE y, BYTE *array, BYTE size) {
		int N = size / 8; //height of the sprite
		int index = 0;
		bool _xor;
		bool cvd = 0;

		for (int j = y; j < y + N; j++) {
			for (int i = x; i < x + 8; i++) {
				_xor = GetPixel(i, j);
				if (_xor == 1 && array[index] == 1) { cvd = 1; }
				PixelSet(i % 64, j % 32, array[index] ^ _xor);
				index++;
			}
		}
		return cvd;
	}
	/*
	void DrawSprite(int x, int y, BYTE *array, int size) {
		int N = size / 8; //height of the sprite
		int index = 0;

		for (int j = y; j < y + N; j++) {
			for (int i = x; i < x + 8; i++) {
				PixelSet(i % 64, j % 32, array[index]);
				index++;
			}
		}*/

		/*if (x + 8 <= 64 && y + N <= 32) {
			for (int j = y; j < y + N; j++) {
				for (int i = x; i < x + 8; i++) {
					PixelSet(i, j, array[index]);
					index++;
				}
			}
		}

		else if (x + 8 > 64 && y + N <= 32) {
			int temp = x + 8 - 64;

			for (int j = y; j < y + N; j++) {
				for (int i = x; i < x + 8 - temp; i++) {
					PixelSet(i, j, array[index]);
					index++;
				}
			}

			for (int j = y; j < y + N; j++) {
				for (int i = 0; i < temp; i++) {
					PixelSet(i, j, array[index]);
					index++;
				}
			}
		}

		else if (x + 8 <= 64 && y + N > 32) {
			int temp = y + N - 32;

			for (int j = y; j < y + N - temp; j++) {
				for (int i = x; i < x + 8; i++) {
					PixelSet(i, j, array[index]);
					index++;
				}
			}

			for (int j = 0; j < temp; j++) {
				for (int i = x; i < x + 8; i++) {
					PixelSet(i, j, array[index]);
					index++;
				}
			}
		}

		else if (x + 8 > 64 && y + N > 32) {
			int temp_x = x + 8 - 64;
			int temp_y = y + N - 32;

			for (int j = y; j < y + N - temp_y; j++) {
				for (int i = x; i < x + 8 - temp_x; i++) {
					PixelSet(i, j, array[index]);
					index++;
				}
			}

			for (int j = 0; j < temp_y; j++) {
				for (int i = 0; i < temp_x; i++) {
					PixelSet(i, j, array[index]);
					index++;
				}
			}
		}
	}*/
};
