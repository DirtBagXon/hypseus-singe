/*
 * serial.cpp
 *
 * Copyright (C) 2001 Matt Ownby
 *
 * This file is part of DAPHNE, a laserdisc arcade game emulator
 *
 * DAPHNE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * DAPHNE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Serial routines for DAPHNE emulator
// by Matt Ownby
// started 16 August 1999

#include <stdio.h>
#include <stdlib.h>
#include "serial.h"
#include "conout.h"
#include "../timer/timer.h"

// WIN32 Specific Stuff Below

#ifdef WIN32
#include <windows.h>
	HANDLE h;
	COMMTIMEOUTS cto = { 2, 1, 1, 0, 0 };
	DCB dcb;
	OVERLAPPED ov;
	unsigned char g_char_in_buf = 0;	// our single buffered character
	bool g_char_buf_filled = false;	// whether we have any character waiting to be read in our buffer
#endif

#ifdef UNIX
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

	struct termios oldterm, newterm;
	int fd = 0;

#endif







#ifdef WIN32

bool serial_init(int port, int baudrate)
// initializes serial port for use
// port is which serial port to open
// port 0 is COM1 or /dev/ttyS0
// port 1 is COM2 or /dev/ttyS1, etc ...
// baudrate is obvious.  example: 9600 baud
// returns a 1 if successful, 0 if error
{
	bool result = true;
	int i = 0;
	char portname[5] = { 0 };

	ZeroMemory(&ov,sizeof(ov));

	char s[81] = { 0 };
	sprintf(s, "Opening serial port %d at %d baud", port, baudrate);
	printline(s);

	sprintf(portname, "COM%d", port+1);

	h = CreateFile(portname,
		GENERIC_READ|GENERIC_WRITE,
		0,	// no shared mode
		NULL,	// pointer to security attributes
		OPEN_EXISTING,	// how to open serial port
		FILE_FLAG_OVERLAPPED,
		NULL);

	if (h == INVALID_HANDLE_VALUE)
	{
         printline("Failed to open serial port!");
		 result = 0;
	}

	else
	{
		i = SetCommTimeouts(h, &cto);
		if (!i)

		{
            printline("SetCommTimeouts failed");
			result = 0;
		}

         // set DCB
		memset(&dcb,0,sizeof(dcb));	// clear dcb record
		dcb.DCBlength = sizeof(dcb);
		dcb.BaudRate = baudrate;
		dcb.fBinary = 1;
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
		dcb.fOutxCtsFlow = 1;
		dcb.fRtsControl = DTR_CONTROL_HANDSHAKE;
		dcb.Parity = NOPARITY;
		dcb.StopBits = ONESTOPBIT;
		dcb.ByteSize = 8;
		dcb.fNull = FALSE;	// FIXME : I'm not sure if I want to do this or not

		if (!SetCommState(h,&dcb))
		{
			printline("SetCommState failed");
			result = 0;
		}
	}

	return(result);
}

int serial_tx(unsigned char ch)
// sends a character (ch) to the open serial port
// returns a 1 if successful, 0 if failed
{
	int result = 0;
	unsigned long writecount = 1;
	OVERLAPPED osWriter = { 0 };
	DWORD dwRes = 0;

	osWriter.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (osWriter.hEvent == NULL)
	{
		printline("Error creating write event");
	}

	if( !WriteFile(h,&ch,writecount,&writecount,&osWriter) )
	{
		if (GetLastError() != ERROR_IO_PENDING)
		{
			printline("Attempt to write failed!");
		}
		else
		{
			dwRes = WaitForSingleObject(osWriter.hEvent, INFINITE);
			switch(dwRes)
				// overlapped structure's event has been signaled
			{
			case WAIT_OBJECT_0:
				// event completed
				if (!GetOverlappedResult(h, &osWriter, &writecount, FALSE))
				{
					printline("Error writing");
				}
				else
				{
					result = 1;
				}
				break;
			default:
				printline("Write error occurred, problem with event handle");
				break;
			}
		}
	}
	else
		// write operation completed immediately
	{
		char s[81];
		sprintf(s, "Wrote %x to serial port", ch);
		printline(s);
		result = 1;
	}
	CloseHandle(osWriter.hEvent);

	return(result);
}

// reads a character from serial port
unsigned char serial_rx()
{
	unsigned char result = 0;

	if (g_char_buf_filled)
	{
		result = g_char_in_buf;
		g_char_buf_filled = false;
		g_char_in_buf = 0;
	}
	else
	{
		printline("SERIAL ERROR : serial port had no character waiting to be read, did you check??");
	}

	return result;
}

// returns true if there is a character waiting to be read
// this should be called before serial_rx
bool serial_rx_char_waiting()
{

	bool result = g_char_buf_filled;

	// if buffer was empty, check to see if it's still empty
	if (!result)
	{

		unsigned char ch = 0;
		DWORD dwRead = 0;
		DWORD dwRes = 0;
		OVERLAPPED osReader = {0};
		BOOL fWaitingOnRead = FALSE;

		osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (osReader.hEvent == NULL)
		{
			printline("Error creating overlapped event");
		}

		if (!ReadFile(h, &ch, sizeof(ch), &dwRead, &osReader))
		{
			if( GetLastError() != ERROR_IO_PENDING )
			{
				printline("Error in read communications");
			}
			else
			{
				fWaitingOnRead = TRUE;

				// loop until we get some data
				while (fWaitingOnRead)
				{

					dwRes = WaitForSingleObject(osReader.hEvent, 500);

					switch(dwRes)
					{
					case WAIT_OBJECT_0:
						// read completed
						if (!GetOverlappedResult(h, &osReader, &dwRead, FALSE))
						{
							printline("Error in read comm");
						}

						else
						{
							// MATT : I changed this from (ch != 0) to try to fix a problem
							// with a byte of '00' not being able to be read from the serial port
							if (dwRead > 0)
							{
								g_char_in_buf = ch;
								g_char_buf_filled = true;
							}
						}

						fWaitingOnRead = FALSE;
						break;
					default:
						printline("Error with OVERLAPPED event handle");
						break;
					}
				}	// end while ...
			}
		} // end if readfile failed

		// if we have read data
		else
		{
			g_char_buf_filled = true;
			g_char_in_buf = ch;
		}

		CloseHandle(osReader.hEvent);

	} // end if buffer was empty

	// else the buffer is full, read something from it!

	return(result);
}

void serial_close()
// closes the open serial port
// returns a 1 if successful, 0 if error
{

	printline("Closing serial port...");

	CloseHandle(h);

}



#endif

// end WIN32 code ...



////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef UNIX

// Unix styled code here ...

bool serial_init(int port, int baudrate)
// initializes serial port for use
// port is which serial port to open
// port 0 is /dev/ttyS0
// port 1 is /dev/ttyS1, etc ...
// baudrate is obvious.  example: 9600 baud
// returns a 1 if successful, 0 if error
{
	bool result = false;
	char portname[81] = { 0 };
	unsigned char baud_symbol = 0;

#ifdef LINUX
	sprintf(portname, "/dev/ttyS%d", port);
#else

#ifdef FreeBSD
	sprintf(portname, "/dev/cuaa%d",port);
#else
	strcpy(portname, "/dev/null");	// define this for your unix'ish OS
#endif

#endif

	if (baudrate == 9600)
	{
		baud_symbol = B9600;
	}

	else if (baudrate == 4800)
	{
		baud_symbol = B4800;
	}

	else
	{
		printline("DAPHNE for unix only supports 9600 and 4800 baud, using 9600 baud.");
		baud_symbol = B9600;
	}

	fd = open(portname, O_RDWR | O_NOCTTY );

	// if we were able to open the serial port
	if (fd)
	{
		// preserve current settings
		if (tcgetattr(fd, &oldterm) != -1)
		{
			bzero(&newterm, sizeof(newterm)); // clear structure
			newterm.c_cflag = baud_symbol | CRTSCTS | CS8 | CLOCAL | CREAD;
			newterm.c_iflag = IGNPAR | ICRNL;
			newterm.c_oflag = 0;
			newterm.c_lflag &= ~ICANON;
			newterm.c_cc[VTIME] = 0;	// block for 1 sec for emergencies
			newterm.c_cc[VMIN] = 1;	// no character

			// flush something <grin> and check for error
			if (tcflush(fd, TCIFLUSH) != -1)
			{

				// setup the port for our new settings and check for any errors
				if (tcsetattr(fd, TCSANOW, &newterm) != -1)
				{
					result = true;
				}
				else
				{
					printline("SERIAL : Error setting new serial attributes!");
				}
			}
			else
			{
				printline("SERIAL : Error flushing serial settings!");
			}
		}
		else
		{
			printline("SERIAL : Error preserving current settings!");
		}
	}
	else
	{
		printline("SERIAL : Error opening serial port!");
	}

	return(result);
}



int serial_tx(unsigned char ch)
// sends a character (ch) to the open serial port
// returns true if successful, false if failed
{
	return (write(fd, &ch, 1));
}

// returns a character from the incoming buffer
// If there is no character available, it returns a 0 and prints an error
// You should check to see whether a character is available first before calling this
unsigned char serial_rx()
{
	unsigned char result = 0;

	if (serial_rx_char_waiting())
	{
		if (read(fd, &result, 1) != 1)
		{
			printline("SERIAL ERROR : Tried to read a byte but no byte was available.  Did your code check first??");
			result = 0;
		}
	}
	else
	{
		printline("SERIAL ERROR : Tried to read a byte and no byte was available, grr!  Check before reading!");
	}
	
	return(result);
}

// returns true if a character is waiting to be retrieved from the input buffer
bool serial_rx_char_waiting()
{
	bool result = false;

	fd_set set;
	struct timeval timeo;

	FD_ZERO(&set);
	FD_SET(fd, &set);

	timeo.tv_sec = 0;
	timeo.tv_usec = 0;

	// if a character is waiting ...
	if (select(fd + 1, &set, NULL, NULL, &timeo) > 0)
	{
		result = true;
	}

	return result;
}

void serial_close()
// closes the open serial port
// returns a 1 if successful, 0 if error
{
	// If we can't restore settings
	if (tcsetattr(fd, TCSANOW, &oldterm) == -1)
	{
		printline("Error restoring settings to their originals in serial_close()");
	}
	close(fd);
}

#endif
// end Unix code ...

// flush the receive buffer
void serial_rxflush()
{
	// loop until we empty the receive buffer
	while (serial_rx_char_waiting())
	{
		serial_rx();	// get a character, throw it away
	}
}

// sends an ASCII string to serial port and terminates with a
// carriage return (0x0D)
void send_tx_string(const char *s)
{
	unsigned int i = 0;

	for (i = 0; i < strlen(s); i++)
	{
		serial_tx(s[i]);
	}
	serial_tx(0x0D);
}

// gets one byte from serial port
// timeout is how many ms to wait before giving
unsigned char serial_get_one_byte(unsigned int timeout)
{
	unsigned char result = 0;
	unsigned int curtime = refresh_ms_time();
	bool timed_out = false;

	// wait for a byte to become available	
	while (!serial_rx_char_waiting())
	{
		if (elapsed_ms_time(curtime) > timeout)
		{
			timed_out = true;
			printline("Error: Timed out waiting for bytes from serial port");
			break;
		}
	}

	if (!timed_out)
	{
		result = serial_rx();
	}

	return(result);
}
