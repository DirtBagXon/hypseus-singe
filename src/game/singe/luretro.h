/*
** $Id$
** Auxiliary functions from Lua API
** See Copyright Notice in lua.h
*/

#define RETRO_PAD      12
#define RETRO_MAXPATH  128

unsigned char get_retropath();

LUA_API void (os_alter_clocker) (lua_State *L);

void lua_set_retropath(unsigned char value);

void lua_retropath(const char *src, char *dst, int len);
