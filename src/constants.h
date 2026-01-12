#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace Constants
{
	static constexpr int FFT_SIZE = 4096;
	static constexpr int BIN_COUNT = FFT_SIZE / 2;
	static constexpr int HOP_SIZE = 512;

	static constexpr int BUCKET_COUNT = 64;
	static constexpr int BAR_SPACING = 1;

	static constexpr float SMOOTHNESS = 30.0f;
	static constexpr float SMEAREDNESS = 5.0f;
}

#endif