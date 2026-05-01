/*
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2026 DirtBagXon
 *
 * This file is part of HYPSEUS SINGE, a laserdisc arcade game emulator
 *
 * HYPSEUS SINGE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HYPSEUS SINGE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdint.h>
#include <stddef.h>

/* Key to file-handle type for luasocket helpers */
#define R(a,b) ((uint32_t)(((a) << (b)) | ((a) >> (32 - (b)))))

typedef uint32_t u32;
typedef uint64_t u64;
typedef uint8_t  u8;

static u64 m(u64 *s)
{
 u64 x = (*s += 0x9E3779B97F4A7C15ULL);

 x ^= (x >> 30);
 x *= 0xBF58476D1CE4E5B9ULL;
 x ^= (x >> 27);
 x *= 0x94D049BB133111EBULL;

 return x ^ (x >> 31);
}

static void q(u32 *a, u32 *b, u32 *c, u32 *d)
{
 *a += *b; *d ^= *a; *d = R(*d, 16);
 *c += *d; *b ^= *c; *b = R(*b, 12);
 *a += *b; *d ^= *a; *d = R(*d, 8);
 *c += *d; *b ^= *c; *b = R(*b, 7);
}

static void b(u32 o[16], const u32 k[8], u32 c, const u32 n[3])
{
 static const u32 cst[4] =
  {0x61707865, 0x3320646e,
   0x79622d32, 0x6b206574};

 u32 s[16];

 for (int i = 0; i < 4; i++) s[i] = cst[i];
 for (int i = 0; i < 8; i++) s[4 + i] = k[i];

 s[12] = c;
 s[13] = n[0];
 s[14] = n[1];
 s[15] = n[2];

 for (int i = 0; i < 16; i++) o[i] = s[i];

 for (int i = 0; i < 10; i++)
 {
  q(&o[0], &o[4], &o[8],  &o[12]);
  q(&o[1], &o[5], &o[9],  &o[13]);
  q(&o[2], &o[6], &o[10], &o[14]);
  q(&o[3], &o[7], &o[11], &o[15]);

  q(&o[0], &o[5], &o[10], &o[15]);
  q(&o[1], &o[6], &o[11], &o[12]);
  q(&o[2], &o[7], &o[8],  &o[13]);
  q(&o[3], &o[4], &o[9],  &o[14]);
 }

 for (int i = 0; i < 16; i++) o[i] += s[i];
}

static u32 L(const u8 *p)
{
 return (u32)p[0]
  | ((u32)p[1] << 8)
  | ((u32)p[2] << 16)
  | ((u32)p[3] << 24);
}

static void S(u8 *p, u32 v)
{
 p[0] = (u8)(v);
 p[1] = (u8)(v >> 8);
 p[2] = (u8)(v >> 16);
 p[3] = (u8)(v >> 24);
}

/*
** ======================================================
** PUSH
** =======================================================
*/

void lua_push(void *d, size_t len, const u8 k[32], const u8 n[12], u32 c)
{
 u8 *p = (u8 *)d;

 u32 kw[8], nw[3];
 u32 bl[16];
 u8  ks[64];

 size_t i, j;
 for (i = 0; i < 8; i++)
  kw[i] = L(k + 4 * i);

 for (i = 0; i < 3; i++)
  nw[i] = L(n + 4 * i);

 while (len) {
  b(bl, kw, c, nw);
  for (i = 0; i < 16; i++)
   S(&ks[i * 4], bl[i]);

  size_t mlen = (len < 64) ? len : 64;
  for (j = 0; j < mlen; j++)
   p[j] ^= ks[j];

  p += mlen;
  len -= mlen;
  c += 1;
 }
}

/*
** this function has a separated environment, which defines the
** correct __open for 'metatable.__index' files
*/

void lua_settab(u64 e, u8 k[32], u8 n[12])
{
 u64 s = e;

 for (int i = 0; i < 32; i += 8) {
  u64 x = m(&s);
  for (int j = 0; j < 8; j++)
   k[i+j] = (u8)(x >> (8 * j));
 }

 for (int i = 0; i < 12; i += 8) {
  u64 x = m(&s);
  for (int j = 0; j < 8 && (i + j) < 12; j++)
   n[i+j] = (u8)(x >> (8 * j));
 }
}
