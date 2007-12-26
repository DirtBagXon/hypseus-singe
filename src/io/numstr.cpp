/*
 * mpo_numstr.cpp
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

#include "numstr.h"

#ifndef WIN32
#include <ctype.h>	// for toupper
#endif

const char *DIGITS = "0123456789ABCDEF";

// NOTE : this function doesn't do full safety checking in the interest of simplicity and speed
int numstr::ToInt32(const char *str)
{
	const int BASE = 10;	// for now we always assume base is 10 because I never seem to use anything else
	int result = 0;
	bool found_first_digit = false;	// whether we have found the first digit or not
	int sign_mult = 1;	// 1 if the number is positive, -1 if it's negative

	for (unsigned int i = 0; i < my_strlen(str); i++)
	{
		if (!found_first_digit)
		{
			if (is_digit(str[i], BASE))
			{
				found_first_digit = true;
			}

			// if it a negative number?
			else if (str[i] == '-')
			{
				sign_mult = -1;
			}

			// else it's an unknown character, so we ignore it until we get to the first digit
		}

		// note: we do not want this to be an "else if" because the above 'if' needs to flow into this
		if (found_first_digit)
		{
			// make sure we aren't dealing with any non-integers
			if ((str[i] >= '0') && (str[i] <= '9'))
			{
				result *= BASE;
				result += str[i] - '0';
			}
			// else we've hit unknown characters so we're done
			else
			{
				break;
			}
		}
	}

	return result * sign_mult;
}

unsigned int numstr::ToUint32(const char *str, int base)
{
	unsigned int u = 0;
	ToUint(str,u,base);
	return u;
}

MPO_UINT64 numstr::ToUint64(const char *str, int base)
{
	MPO_UINT64 u = 0;
	ToUint(str,u,base);
	return u;
}

double numstr::ToDouble(const char *str)
{
	const double BASE = 10.0;	// this makes it easier for now ...
	const double BASE_DIVIDE = 0.1;	// because multiplaying by 0.1 is faster than dividing by 10.0
	bool found_period = false;	// whether we've encountered the period in the double yet
	bool found_first_digit = false;
	double result = 0.0;
	double divide_by = 1.0;	// after we pass the decimal point, we need to divide subsequent numbers to put them in their proper sphere
	double sign_mult = 1.0;	// final result is multiplied by this to set the sign

	for (unsigned int i = 0; i < my_strlen(str); i++)
	{
		if (!found_first_digit)
		{
			if (is_digit(str[i], (int) BASE))
			{
				found_first_digit = true;
			}

			// if it a negative number?
			else if (str[i] == '-')
			{
				sign_mult = -1.0;
			}

			else if (str[i] == '.')
			{
				found_period = true;
			}

			// else it's an unknown character, so we ignore it until we get to the first digit
		}

		// note: we do not want this to be an "else if" because the above 'if' needs to flow into this
		if (found_first_digit)
		{
			// make sure we aren't dealing with any non-integers
			if ((str[i] >= '0') && (str[i] <= '9'))
			{
				// if we haven't encountered the decimal place yet
				if (found_period == false)
				{
					result *= BASE;
					result += str[i] - '0';
				}
				// if we are passed the decimal place
				else
				{
					divide_by *= BASE_DIVIDE;	// each new digit we find needs to be divided by 10x the previous divide_by value
					result += ((str[i] - '0') * divide_by);
				}
			}
			// else if we've hit the decimal point
			else if (str[i] == '.')
			{
				found_period = true;
			}
			// else we've hit unknown characters so we're done
			else
			{
				break;
			}
		}
	}

	return result * sign_mult;
}

// DUMMY functions to force template instantiation
// (since I can't figure out how to make the compiler do this without! arrgh)

string numstr::ToStr(int num, int base, unsigned int min_digits)
{
	unsigned int dummy = 0;	// see comment in .h for IToStr for explanation
	return IToStr(num, dummy, base, min_digits);
}

string numstr::ToStr(MPO_INT64 num, int base, unsigned int min_digits)
{
	MPO_UINT64 dummy = 0;	// see comment in .h for IToStr for explanation
	return IToStr(num, dummy, base, min_digits);
}

string numstr::ToStr(unsigned char c, int base, unsigned int min_digits)
{
	return UToStr(c, base, min_digits);
}

string numstr::ToStr(unsigned int u, int base, unsigned int min_digits)
{
	return UToStr(u, base, min_digits);
}

string numstr::ToStr(MPO_UINT64 u, int base, unsigned int min_digits)
{
	return UToStr(u, base, min_digits);
}

string numstr::ToStr(double d, unsigned int min_digits_before, unsigned int min_digits_after, unsigned int max_digits_after)
{
	string result = "(overflow)";
	const double BASE = 10.0;	// we will only support base 10 with doubles to simplify things ...
	unsigned int decimal_length = 0;

	// bounds check: make sure the double is within our limits (2^62 was the highest I could get it to work without introducing overflow errors)
	if ((d <= 4611686018427387904.0) && (d >= -4611686018427387904.0))
	{
		MPO_INT64 int64_portion = (MPO_INT64) d;	// strip off floating point part
		d = d - int64_portion;	// isolate just the decimal portion

		result = ToStr(int64_portion, 10, min_digits_before);	// use our other function to calculate the int portion

		result = result + ".";	// add decimal place, it will always be displayed even if there is no fractional value to this number

		if (d < 0) d *= -1.0;	// force d to be positive

		// NOTE : d will now always be positive
		do
		{
			d *= BASE;	// move decimal point one notch to the right
			int int_portion = (int) d;	// grab the number that is above the decimal point
			result = result + DIGITS[int_portion];
			d = d - int_portion;
			decimal_length++;	// gotta keep track of this for 'min_digits_after' calculation
		} while ((d != 0.0) && (max_digits_after > decimal_length));

		while (decimal_length < min_digits_after)
		{
			result = result + "0";	// pad trailing zeroes
			decimal_length++;
		}
	}
	// else return default result to indicate error

	return result;
}

string numstr::ToUnitStr(MPO_UINT64 u)
{
	string result;
	double d;

	// if less than 1 k
	if (u < 1024)
	{
		result = ToStr(u) + " B";
	}

	// less than 1 meg
	else if (u < 1048576)
	{
		d = u * 0.0009765625;	// same as dividing by 1024
		result = ToStr(d, 0, 1, 2) + " KiB";
	}

	// less than 1 gig
	else if (u < 1073741824)
	{
		d = u / (1048576.0);	// convert to megs
		result = ToStr(d, 0, 1, 2) + " MiB";
	}

	// else leave it as gigs, we won't go any higher for now
	else
	{
		d = u / (1073741824.0);	// convert to gigs
		result = ToStr(d, 0, 1, 2) + " GiB";
	}

	return result;
}

unsigned int numstr::my_strlen(const char *s)
{
	unsigned int i = 0;
	while (s[i] != 0) i++;
	return i;
}

////////////////////////////////
// private funcs

inline bool numstr::is_digit(char ch, int base)
{
	if ((base == 10) && (ch >= '0') && (ch <= '9')) return(true);
	else if ((base == 16) && (
		((ch >= '0') && (ch <= '9')) || 
		((toupper(ch) >= 'A') && (toupper(ch) <= 'F'))
		)) return(true);
	// else no other base is supported

	return false;
}
