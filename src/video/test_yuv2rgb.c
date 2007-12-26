#include <stdio.h>
#include <stdlib.h>
#include "yuv2rgb.h"

int clamp(int i)
{
	int iRes = i;
	if (i > 255) iRes = 255;
	else if (i < 0) iRes = 0;
	return iRes;
}

int main(int argc, char **argv)
{
	int iThreshold = 1;
	int y = 0, v = 0, u = 0;

	for (y = 0; y < 256; y++)
	{
		for (v = 0; v < 256; v++)
		{
			for (u = 0; u < 256; u++)
			{
				int my_r = clamp((int) (1.164*(y - 16) + 1.596*(v-128)));
				int my_g = clamp((int) (1.164*(y - 16) - (0.813*(v-128)) - (0.392*(u-128))));
				int my_b = clamp((int) (1.164*(y - 16) + (2.017*(u-128))));

				yuv2rgb_input[0] = y;
				yuv2rgb_input[1] = u;
				yuv2rgb_input[2] = v;
				yuv2rgb();

				if (abs(my_r - yuv2rgb_result_r) > iThreshold)
				{
					printf("[Y:%3d U:%3d V:%3d] - my_r is %d, res_r is %d\n", y, u, v, my_r, yuv2rgb_result_r);
				}
				if (abs(my_g - yuv2rgb_result_g) > iThreshold)
				{
					printf("[Y:%3d U:%3d V:%3d] - my_g is %d, res_g is %d\n", y, u, v, my_g, yuv2rgb_result_g);
				}
				if (abs(my_b - yuv2rgb_result_b) > iThreshold)
				{
					printf("[Y:%3d U:%3d V:%3d] - my_b is %d, res_b is %d\n", y, u, v, my_b, yuv2rgb_result_b);
				}

			}
		}
	}
	return 0;
}
