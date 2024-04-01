/*
 * Copyright (c) 2011 Petr Stetiar <ynezz@true.cz>, Gaben Ltd.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __LIBRS232_POSIX_H__
#define __LIBRS232_POSIX_H__

#include <termios.h>

#ifndef B460800
#define B460800 460800
#endif
#ifndef B921600
#define B921600 921600
#endif

struct rs232_posix_t {
	int fd;
	struct termios oldterm;
};

#define GET_PORT_STATE(fd, state) \
	if (tcgetattr(fd, state) < 0) { \
		DBG("tcgetattr() %d %s\n", errno, strerror(errno)) \
		return RS232_ERR_CONFIG; \
	}

#define SET_PORT_STATE(fd, state) \
	if (tcsetattr(fd, TCSANOW, state) < 0) { \
		DBG("tcsetattr() %d %s\n", errno, strerror(errno)) \
		return RS232_ERR_CONFIG; \
	} \

#endif /* __LIBRS232_POSIX_H__ */
