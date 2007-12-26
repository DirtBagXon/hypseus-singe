#include <stdio.h>
#include <string.h>	// for memcpy
#include "interface_920.h"

int main(int argc, char **argv)
{
	if (argc == 2)
	{
		FILE *F = fopen(argv[1], "rb");
		if (F)
		{
			unsigned char buf[4096];
			mp2_920_init();

			for (;;)
			{
				size_t uBytesRead = fread(buf, 1, sizeof(buf), F);

				mp2_920_decode_mpeg(buf, uBytesRead);

				// we're done!
				if (uBytesRead < sizeof(buf))
				{
					break;
				}
			}
			fclose(F);

			mp2_920_shutdown();
		}
		else
		{
			printf("File %s could not be opened\n", argv[1]);
		}
	}
	else
	{
		printf("usage: %s <mpeg stream name>\n", argv[0]);
	}

	printf("Main returning\n");

	return 0;
}

