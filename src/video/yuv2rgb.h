#ifndef YUV2RGB_H
#define YUV2RGB_H

#ifdef __cplusplus
extern "C" {
#endif // c++

#include "yuv2rgb_lookup.h"

extern unsigned int yuv2rgb_input[3];	// 8-bit values
extern unsigned int yuv2rgb_result_r;	// 8-bit
extern unsigned int yuv2rgb_result_g;	// 8-bit
extern unsigned int yuv2rgb_result_b;	// 8-bit

#define CLAMP(i) ((i > 255) ? 255 : ((i < 0) ? 0 : i))

#define YUV2RGB(Y, U, V, R, G, B) \
	R = (Y1164MIN16LS[(Y)] + V1596MIN128LS[(V)]) >> 15;	\
	G = (Y1164MIN16LS[(Y)] - V0813MIN128LS[(V)] - U0392MIN128LS[(U)]) >> 15;	\
	B = (Y1164MIN16LS[(Y)] + U2017MIN128LS[(U)]) >> 15;	\
	R = (unsigned int) CLAMP((int)R); \
	G = (unsigned int) CLAMP((int)G); \
	B = (unsigned int) CLAMP((int)B)

void yuv2rgb();

#ifdef __cplusplus
}
#endif // c++

#endif // YUV2RGB_H
