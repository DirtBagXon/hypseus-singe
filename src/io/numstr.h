/*
 * mpo_numstr.h
 *
 * Copyright (C) 2005 Matthew P. Ownby
 *
 * This file is part of MPOLIB, a multi-purpose library
 *
 * MPOLIB is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPOLIB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// MPO's NOTE:
//  I may wish to use MPOLIB in a proprietary product some day.  Therefore,
//   the only way I can accept other people's changes to my code is if they
//   give me full ownership of those changes.

// numstr.h
// by Matt Ownby

#ifndef NUMSTR_H
#define NUMSTR_H

#ifdef WIN32
typedef unsigned __int64 MPO_UINT64;
typedef __int64 MPO_INT64;
#else
typedef unsigned long long MPO_UINT64;
typedef long long MPO_INT64;
#endif

#include <string>

using namespace std;

class numstr
{
public:
	static int ToInt32(const char *str);
	static unsigned int ToUint32(const char *str, int base = 10);
	static MPO_UINT64 ToUint64(const char *str, int base = 10);
	static double ToDouble(const char *s);
	static string ToStr(int i, int base = 10, unsigned int min_digits = 0);
	static string ToStr(MPO_INT64 num, int base = 10, unsigned int min_digits = 0);
	static string ToStr(unsigned int u, int base = 10, unsigned int min_digits = 0);
	static string ToStr(unsigned char u, int base = 10, unsigned int min_digits = 0);
	static string ToStr(MPO_UINT64 u, int base = 10, unsigned int min_digits = 0);
	
	// NOTE : double cannot be > 2^63 (size of signed 64-bit int) or this conversion will fail
	static string ToStr(double d, unsigned int min_digits_before = 0, unsigned int min_digits_after = 0, unsigned int max_digits_after = 5);

	// converts raw bytes to KiB, MiB, or GiB to make it more readable
	static string ToUnitStr(MPO_UINT64 u);
	
	static unsigned int my_strlen(const char *s);
private:
	inline static bool is_digit(char ch, int base);
	
	// NOTE : this is put in the .h because VC++ 6.0 can't handle it any other way
	// convert a string to an unsigned number
	template <class T> static void ToUint(const char *str, T &result, int base)
	{
		bool found_first_digit = false;	// whether we have found the first digit or not
		
		result = 0;
		
		// go through each digit
		for (unsigned int i = 0; i < my_strlen(str); i++)
		{
			if (!found_first_digit)
			{
				if (is_digit(str[i], base))
				{
					found_first_digit = true;
				}
				
				// else it's an unknown character, so we ignore it until we get to the first digit
			}
			
			// note: we do not want this to be an "else if" because the above 'if' needs to flow into this
			if (found_first_digit)
			{
				// converting from base10 ASCII
				if ((base == 10) && (str[i] >= '0') && (str[i] <= '9'))
				{
					result *= 10;
					result += str[i] - '0';
				}
				// converting from HEX ASCII
				else if (base == 16)
				{
					// if the number is between '0' and '9'
					if ((str[i] >= '0') && (str[i] <= '9'))
					{
						result *= 16;
						result += str[i] - '0';
					}
					// if the number is between 'A' and 'F'
					else if ((toupper(str[i]) >= 'A') && (toupper(str[i]) <= 'F'))
					{
						result *= 16;
						result += (toupper(str[i]) - 'A') + 10;	// A is the same as 10 decimal
					}
				}
				// else other bases are unsupported
				else
				{
					break;
				}
			}
		}
	}

	// NOTE : this is put in the .h file because VC++ 6.0 can't handle it any other way
	// convert unsigned number to string
	template <class T> static string UToStr(T u, int base = 10, unsigned int min_digits = 0)
	{
		string result = "";
		const char *DIGITS = "0123456789ABCDEF";
		
		do
		{
			result = DIGITS[(u % base)] + result;
			u = u / base;
		} while (u != 0);
		
		// pad the front of the number with 0's to satisfy the min_digits requirement
		while (result.length() < min_digits)
		{
			result = "0" + result;
		}
		
		return result;		
	}

	// NOTE : this is put in the .h file because VC++ 6.0 can't handle it any other way
	// convert unsigned number to string
	// NOTE #2: the UT class is just an unsigned version of the T class because I couldn't
	//  figure out how to do (unsigned T) in this function without getting a syntax error.
	template <class T, class UT> static string IToStr(T num, UT unsigned_num, int base = 10, unsigned int min_digits = 0)
	{
		string result = "";
		const char *DIGITS = "0123456789ABCDEF";
		bool negative = false;	// whether the number is negative
		UT i = 0;

		// we will only support negative numbers with base 10
		if (base == 10)
		{
			if (num < 0)
			{
				num *= -1;	// make it positive
				negative = true;
			}
		}

		i = (UT) num;	// now convert it to an unsigned number. this makes negative numbers positive for non-base 10 representations

		do
		{
			result = DIGITS[(i % base)] + result;
			i = i / base;
		} while (i != 0);

		// pad the front of the number with 0's to satisfy the min_digits requirement
		while (result.length() < min_digits)
		{
			result = "0" + result;
		}

		if (negative) result = "-" + result;	// add - sign to the front

		return result;
	}


};

#endif	// NUMSTR_H
