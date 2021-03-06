#pragma once

#include "DataTypes.h"
#include <array>
#include <SFML/Graphics.hpp>
#include <string>

class Gpu {
private:

	inline static std::array<BYTE, 64 * 32> frame_buffer;

	inline static sf::Sprite basic_pixel;
public:

	Gpu() {
		BufferReset();
		sf::Texture p;
		p.create(20, 20);

		BYTE temp_arr[20 * 20 * 4];
		memset(temp_arr, 255, sizeof(BYTE) * 20 * 20 * 4);
		p.update(temp_arr);
		Gpu::basic_pixel.setTexture(p);
	}

	bool GetPixel(BYTE x, BYTE y) {
		return frame_buffer[(y % 32) * 64 + x % 64];
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
	bool DrawSprite(BYTE x, BYTE y, BYTE *array, BYTE N) {
		bool old_pixel = false;
		bool ret = false;
		BYTE new_pixel;
		BYTE row;
		for (int i = 0; i < N; i++) {
			row = array[i];
			for (int j = 7; j >= 0; j--) {
				old_pixel = GetPixel(x + 7 - j, y + i);
				new_pixel = (row >> j) & 0x1;
				if (old_pixel == 1 && new_pixel == 1 && !ret)
					ret = true;

				PixelSet((x + 7 - j) % 64, (y + i) % 32, old_pixel ^ new_pixel);
			}
		}
		return ret;
	}

	static void rendering(sf::RenderWindow* window) {
		window->setActive(true);

		while (window->isOpen()) {
			window->clear(sf::Color::Black);

			for (int y = 0; y < 32; y++) {
				for (int x = 0; x < 64; x++) {
					Gpu::basic_pixel.setPosition(sf::Vector2f(x * 20, y * 20));
					if (Gpu::frame_buffer[y * 64 + x]) {
						Gpu::basic_pixel.setColor(sf::Color::White);
					}
					else {
						Gpu::basic_pixel.setColor(sf::Color::Black);
					}
					window->draw(Gpu::basic_pixel);
				}
			}

			window->display();
		}
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
