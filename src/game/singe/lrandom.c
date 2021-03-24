/*
* lrandom.c
* random-number library for Lua 5.1 based on the Mersenne Twister
* Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br>
* 17 Mar 2010 12:31:47
* This code is hereby placed in the public domain.
*/

#include <math.h>
#include <stdio.h>

#include "lua.h"
#include "lauxlib.h"

#define MYNAME		"random"
#define MYVERSION	MYNAME " library for " LUA_VERSION " / Mar 2010"
#define MYTYPE		MYNAME " handle"

#define SEED		2009UL

#include "random.c"

static MT *Pget(lua_State *L, int i)
{
 return luaL_checkudata(L,i,MYTYPE);
}

static MT *Pnew(lua_State *L)
{
 MT *c=lua_newuserdata(L,sizeof(MT));
 luaL_getmetatable(L,MYTYPE);
 lua_setmetatable(L,-2);
 return c;
}

static int Lnew(lua_State *L)			/** new([seed]) */
{
 lua_Number seed=luaL_optnumber(L,1,SEED);
 MT *c=Pnew(L);
 init_genrand(c,seed);
 return 1;
}

static int Lclone(lua_State *L)			/** clone(c) */
{
 MT *c=Pget(L,1);
 MT *d=Pnew(L);
 *d=*c;
 return 1;
}

static int Lseed(lua_State *L)			/** seed(c,[seed]) */
{
 MT *c=Pget(L,1);
 init_genrand(c,luaL_optnumber(L,2,SEED));
 lua_settop(L,1);
 return 1;
}

static int Lvalue(lua_State *L)			/** value(c,[a,b]) */
{
 MT *c=Pget(L,1);
 double a,b,r=genrand_res53(c);
 switch (lua_gettop(L))
 {
  case 1:
   lua_pushnumber(L,r);
   return 1;
  case 2:
   a=1;
   b=luaL_checknumber(L,2);
   break;
  default:
   a=luaL_checknumber(L,2);
   b=luaL_checknumber(L,3);
   break;
 }
 if (a>b) { double t=a; a=b; b=t; }
 a=ceil(a);
 b=floor(b);
 if (a>b) return 0;
 r=a+floor(r*(b-a+1));
 lua_pushnumber(L,r);
 return 1;
}

static int Ltostring(lua_State *L)
{
 MT *c=Pget(L,1);
 lua_pushfstring(L,"%s %p",MYTYPE,(void*)c);
 return 1;
}

static const luaL_reg R[] =
{
	{ "__tostring",	Ltostring	},	/** __tostring(c) */
	{ "clone",	Lclone		},
	{ "new",	Lnew		},
	{ "seed",	Lseed		},
	{ "value",	Lvalue		},
	{ NULL,		NULL		}
};

LUALIB_API int luaopen_random(lua_State *L)
{
 luaL_newmetatable(L,MYTYPE);
 lua_setglobal(L,MYNAME);
 luaL_register(L,MYNAME,R);
 lua_pushliteral(L,"version");			/** version */
 lua_pushliteral(L,MYVERSION);
 lua_settable(L,-3);
 lua_pushliteral(L,"__index");
 lua_pushvalue(L,-2);
 lua_settable(L,-3);
 lua_pushliteral(L,"__call");			/** __call(c) */
 lua_pushliteral(L,"value");
 lua_gettable(L,-3);
 lua_settable(L,-3);
 return 1;
}
