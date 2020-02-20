#pragma once

#include "DataTypes.h"
#include <array>

class Gpu {
private:

	std::array<BYTE, 64 * 32> frame_buffer;

public:

	Gpu() {}

	//reset frame buffer
	void BufferReset() {
		std::fill(frame_buffer.begin(), frame_buffer.end(), 0);
	}


	//settare pixel generico
	void PixelSet(int x, int  y, bool choise) {

		frame_buffer[y * 64 + x] = choise;
	}


	//ingresso un array di byte e la sua dim.
	void DrawSprite(int x, int y, BYTE array[], int size) {

		int N = size / 8; //height of the sprite

		if (x + 8 <= 64 && y+N <= 32) {

			for (int i = x; i <= x + 8; i++) {
				for (int j = y; i <= y + N; j++) {

					PixelSet(i, j, 0);

				}

			}
		}

		else if (x + 8 > 64 && y + N <= 32) {

			int temp = x + 8 - 64;

			for (int i = x; i <= x + 8- temp; i++) {
				for (int j = y; i <= y + N; j++) {

					PixelSet(i, j, 0);

				}

			}

			for (int i = 0; i <= temp; i++) {
				for (int j = y; i <= y + N; j++) {

					PixelSet(i, j, 0);

				}

			}
		}

		else if (x + 8 <= 64 && y + N > 32) {

			int temp = y + N - 32;

			for (int i = x; i <= x + 8; i++) {
				for (int j = y; i <= y + N - temp; j++) {

					PixelSet(i, j, 0);

				}

			}

			for (int i = x; i <= x + 8; i++) {
				for (int j = 0; i <= temp; j++) {

					PixelSet(i, j, 0);

				}

			}
		}

		else if (x + 8 > 64 && y + N > 32) {

			int temp_x = x + 8 - 64;
			int temp_y = y + N - 32;

			for (int i = x; i <= x + 8 - temp_x; i++) {
				for (int j = y; i <= y + N - temp_y; j++) {

					PixelSet(i, j, 0);

				}

			}

			for (int i = 0; i <= temp_x; i++) {
				for (int j = 0; i <= temp_y; j++) {

					PixelSet(i, j, 0);

				}

			}
		}

	}

};
