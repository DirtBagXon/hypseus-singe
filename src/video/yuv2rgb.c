#include "yuv2rgb.h"

// these are int's so they are the optimal size for the CPU to work with
unsigned int yuv2rgb_input[3] = { 0 };	// 8-bit values
unsigned int yuv2rgb_result_r = 0;	// 8-bit
unsigned int yuv2rgb_result_g = 0;	// 8-bit
unsigned int yuv2rgb_result_b = 0;	// 8-bit

#define R yuv2rgb_result_r
#define G yuv2rgb_result_g
#define B yuv2rgb_result_b

#define Y yuv2rgb_input[0]
#define U yuv2rgb_input[1]
#define V yuv2rgb_input[2]

// clamps the integer values (it would be better to have conversion formulas that don't require clamping)
#define CLAMP(i) ((i > 255) ? 255 : ((i < 0) ? 0 : i))

void yuv2rgb()
{
	int r = (Y1164MIN16LS[Y] + V1596MIN128LS[V]) >> 15;
	int g = (Y1164MIN16LS[Y] - V0813MIN128LS[V] - U0392MIN128LS[U]) >> 15;
	int b = (Y1164MIN16LS[Y] + U2017MIN128LS[U]) >> 15;

	R = (unsigned int) CLAMP(r);
	G = (unsigned int) CLAMP(g);
	B = (unsigned int) CLAMP(b);
}
