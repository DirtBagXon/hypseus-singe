// generates the freez80table.h files
// by Matt Ownby

#include <stdio.h>

#include "freez80.h"

int main()
{
	FILE *F = NULL;
	int ch = 0;
	unsigned char value = 0;
	
	F = fopen("freez80tables.h", "wb");	// create file
	if (F)
	{
		// make header for increment table
		
		fprintf(F, "static Uint8 m80_inc_flags[256] = {%c%c", 13, 10);
		
		// generate increment table
		for (ch = 0; ch <= 0xFF; ch++)
		{
			value = 0;
			
			// check for H_FLAG
			if ((ch & 0xF) == 0x0)
			{
				value |= H_FLAG;
			}

			// if hi-bit is set, set Signed flag
			if (ch & 0x80)
			{
				value |= S_FLAG;
				
				// if our value is exactly 0x80, set oVerflow flag
				if (ch == 0x80)
				{
					value |= V_FLAG;
				}
			}
			// else if the value is 0, set the Zero Flag
			else if (ch == 0)
			{
				value |= Z_FLAG;
			}
			
			// add undocumented X/Y flags
			value |= (ch & (U3_FLAG | U5_FLAG));
			
			fprintf(F, "	0x%x, // 0%x%c%c", value, ch, 13, 10);
		} // end for
		
		fprintf(F, "};%c%c", 13, 10);
		
		// now make decrement tables

		fprintf(F, "static Uint8 m80_dec_flags[256] = {%c%c", 13, 10);
		
		// generate decrement table
		for (ch = 0; ch <= 0xFF; ch++)
		{
			value = N_FLAG;	// decrementing always sets the N flag
			
			// check for H_FLAG
			if ((ch & 0xF) == 0xF)
			{
				value |= H_FLAG;
			}

			// if hi-bit is set, set Signed flag
			if (ch & 0x80)
			{
				value |= S_FLAG;				
			}
			// else if the value is 0, set the Zero Flag
			else if (ch == 0)
			{
				value |= Z_FLAG;
			}
			// if our value is exactly 0x7F, set the oVerflow flag
			else if (ch == 0x7F)
			{
				value |= V_FLAG;
			}

			// add undocumented X and Y flags			
			value |= (ch & (U3_FLAG | U5_FLAG));
			
			fprintf(F, "	0x%x, // 0%x%c%c", value, ch, 13, 10);
		} // end for

		fprintf(F, "};%c%c", 13, 10);

		// now make S_FLAG/Z_FLAG/U5_FLAG/U3_FLAG tables

		fprintf(F, "static Uint8 m80_sz53_flags[256] = {%c%c", 13, 10);

		for (ch = 0; ch <= 0xFF; ch++)
		{
			value = ch & (U3_FLAG | U5_FLAG);	// do undocumented flags

			// if hi-bit is set, set Signed flag
			if (ch & 0x80)
			{
				value |= S_FLAG;				
			}
			// else if the value is 0, set the Zero Flag
			else if (ch == 0)
			{
				value |= Z_FLAG;
			}
			
			fprintf(F, "	0x%x, // 0%x%c%c", value, ch, 13, 10);
		} // end for
		
		fprintf(F, "};%c%c", 13, 10);

		// now make S_FLAG/Z_FLAG/U5_FLAG/U3_FLAG/P_FLAG tables

		fprintf(F, "static Uint8 m80_sz53p_flags[256] = {%c%c", 13, 10);

		for (ch = 0; ch <= 0xFF; ch++)
		{
			int num_ones = 0;	// how many 1's are in the byte
			int bits_processed = 0;	// how many bits we've processed per byte
			unsigned char ch_copy = ch;	// copy of ch so we don't have to clobber
			value = ch & (U3_FLAG | U5_FLAG);	// undocumented flags

			// if hi-bit is set, set Signed flag
			if (ch & 0x80)
			{
				value |= S_FLAG;				
			}
			// else if the value is 0, set the Zero Flag
			else if (ch == 0)
			{
				value |= Z_FLAG;
			}
			
			// count each bit of each byte
			for (bits_processed = 0; bits_processed < 8; bits_processed++)
			{
				// if bit 0 is set, increase the count
				if ((ch_copy & 1) == 1)
				{
					num_ones++;
				}
				ch_copy = ch_copy >> 1;
			}
			
			// if the number of ones in the byte are even, then set the parity bit
			if ((num_ones % 2) == 0)
			{
				value |= P_FLAG;
			}
			
			fprintf(F, "	0x%x, // 0%x%c%c", value, ch, 13, 10);
		} // end for
		
		fprintf(F, "};%c%c", 13, 10);


		// now make S_FLAG/Z_FLAG/U5_FLAG/H_FLAG/U3_FLAG/P_FLAG tables for bit operations

		fprintf(F, "static Uint8 m80_szhp_bit_flags[256] = {%c%c", 13, 10);

		for (ch = 0; ch <= 0xFF; ch++)
		{
			value = H_FLAG;	// H_FLAG is always set, N flag is always clear
			
			// if ch is not zero,
			if (ch)
			{
				value |= ch & S_FLAG;
				// possibly set flag 7
			}
			// otherwise,
			else
			{
				value |= Z_FLAG | P_FLAG;
				// set Z and P flag if number (bit) is 0
			}
						
			fprintf(F, "	0x%x, // 0%x%c%c", value, ch, 13, 10);
		} // end for
		
		fprintf(F, "};%c%c", 13, 10);

		
	} // end if file was opened successfully
}