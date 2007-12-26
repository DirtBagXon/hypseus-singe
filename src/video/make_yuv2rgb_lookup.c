#include <stdio.h>

// this program generates C lookup tables for YUV2RGB conversion

// Reference formula is:
/*
	R = 1.164(Y-16) + 1.596(V-128)
	G = 1.164(Y-16) - 0.813(V-128) - 0.392(U-128)
	B = 1.164(Y-16) + 2.017(U-128)
*/

int main(int argc, char **argv)
{
	const unsigned int uLeftShift = 15;	// to preserve precision using integers
	unsigned int uLSVal = 1 << uLeftShift;
	int i = 0;

	printf("int Y1164MIN16LS[256] = {\n");
	for (i = 0; i < 256; i++)
	{
		int iRes = (int) (1.164 * (i - 16) * uLSVal);
		printf("%d, ", iRes);
		if ((i % 8) == 0) printf("\n");	// linefeed every so often ...
	}
	printf("};\n\n");

	printf("int V1596MIN128LS[256] = {\n");
	for (i = 0; i < 256; i++)
	{
		int iRes = (int) (1.596 * (i - 128) * uLSVal);
		printf("%d, ", iRes);
		if ((i % 8) == 0) printf("\n");	// linefeed every so often ...
	}
	printf("};\n\n");

	printf("int V0813MIN128LS[256] = {\n");
	for (i = 0; i < 256; i++)
	{
		int iRes = (int) (0.813 * (i - 128) * uLSVal);
		printf("%d, ", iRes);
		if ((i % 8) == 0) printf("\n");	// linefeed every so often ...
	}
	printf("};\n\n");

	printf("int U0392MIN128LS[256] = {\n");
	for (i = 0; i < 256; i++)
	{
		int iRes = (int) (0.392 * (i - 128) * uLSVal);
		printf("%d, ", iRes);
		if ((i % 8) == 0) printf("\n");	// linefeed every so often ...
	}
	printf("};\n\n");

	printf("int U2017MIN128LS[256] = {\n");
	for (i = 0; i < 256; i++)
	{
		int iRes = (int) (2.017 * (i - 128) * uLSVal);
		printf("%d, ", iRes);
		if ((i % 8) == 0) printf("\n");	// linefeed every so often ...
	}
	printf("};\n\n");

	return 0;
}
