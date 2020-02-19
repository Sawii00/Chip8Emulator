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
	void PixelSey(x, y, 1) {
		frame_buffer[x]
	}

	//ingresso un array di byte e la sua dim.
};
