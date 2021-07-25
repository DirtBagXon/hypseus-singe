/*
** $Id$
** Retro path library
** See Copyright Notice in lua.h
*/

static unsigned char g_retropath = 0;

unsigned char get_retropath() { return g_retropath; }

void lua_set_retropath(unsigned char value) { g_retropath = value; }

void lua_retropath(const char *src, char *dst, int len)
{
    unsigned char r = 0;
    for (int i = 0; i < len; src++, i++) {

        if (i == 6) {
            *dst   =  47;
            dst++;
            *dst   =  46;
            dst++;
            *dst   =  46;
            dst++;
            *dst   =  47;
            dst++;
        }
        if (*src == 0x2F && r<0x10) {
            r++;
            continue;
        }
        if (r == 2) {
            *dst   =  46;
            dst++;
            *dst   = 100;
            dst++;
            *dst   =  97;
            dst++;
            *dst   = 112;
            dst++;
            *dst   = 104;
            dst++;
            *dst   = 110;
            dst++;
            *dst   = 101;
            dst++;
            *dst   =  47;
            dst++;
            r = 0x10;
        }
        *dst = *src;
        dst++;
    }
}
