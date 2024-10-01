/**
*  $Id: md5.h,v 1.2 2006/03/03 15:04:49 tomas Exp $
*  Cryptographic module for Lua.
*  @author  Roberto Ierusalimschy
*/


#ifndef md5_h
#define md5_h

#include "lua.h"
#include <stddef.h>


#define HASHSIZE       16

#if __STDC_VERSION__ >= 199901L
#include <stdint.h>
typedef uint32_t WORD32;
#else
/* static assert that int equal or greater than 32bit. */
typedef char static_assert_sizeof_int
[sizeof(unsigned int) >= 4 ? 1 : -1];
typedef unsigned int WORD32;
#endif

typedef struct md5_t {
    WORD32 d[4];
    WORD32 c[2];
    unsigned char in[64];
} md5_t;

void md5_init(md5_t *m);
void md5_update(md5_t *m, const unsigned char *message, size_t len);
void md5_finish(md5_t *m, unsigned char output[HASHSIZE]);
void md5(const unsigned char *message, size_t len, unsigned char output[HASHSIZE]);

LUALIB_API int luaopen_md5_core (lua_State *L);


#endif
