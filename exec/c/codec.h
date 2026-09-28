/* puff.h
  Copyright (C) 2002-2013 Mark Adler, all rights reserved
  version 2.3, 21 Jan 2013

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler    madler@alumni.caltech.edu
 */
/* Altered from Mark Adler puff 2.3: bounded error returns, 9-bit prefix
 * lookup, overlapping pointer copy, exact extents and slice-by-4 CRC32.
 * Generic byte codec only; no language rules. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NIL 0

#define local static

#define MAXBITS 15
#define MAXLCODES 286
#define MAXDCODES 30
#define MAXCODES (MAXLCODES+MAXDCODES)
#define FIXLCODES 288

struct state {

    unsigned char *out;
    unsigned long outlen;
    unsigned long outcnt;

    const unsigned char *in;
    unsigned long inlen;
    unsigned long incnt;
    int bitbuf;
    int bitcnt;

    int error;
};

local int bits(struct state *s, int need)
{
    long val;

    val = s->bitbuf;
    while (s->bitcnt < need) {
        if (s->incnt == s->inlen) { s->error = 2; return 0; }
        val |= (long)(s->in[s->incnt++]) << s->bitcnt;
        s->bitcnt += 8;
    }

    s->bitbuf = (int)(val >> need);
    s->bitcnt -= need;

    return (int)(val & ((1L << need) - 1));
}

local int stored(struct state *s)
{
    unsigned len;

    s->bitbuf = 0;
    s->bitcnt = 0;

    if (s->incnt + 4 > s->inlen)
        return 2;
    len = s->in[s->incnt++];
    len |= s->in[s->incnt++] << 8;
    if (s->in[s->incnt++] != (~len & 0xff) ||
        s->in[s->incnt++] != ((~len >> 8) & 0xff))
        return -2;

    if (s->incnt + len > s->inlen)
        return 2;
    if (s->out != NIL) {
        if (s->outcnt + len > s->outlen)
            return 1;
        while (len--)
            s->out[s->outcnt++] = s->in[s->incnt++];
    }
    else {
        s->outcnt += len;
        s->incnt += len;
    }

    return 0;
}

struct huffman {
    short *count;
    short *symbol;
};

local int decode(struct state *s, const struct huffman *h)
{
    int len;
    int code;
    int first;
    int count;
    int index;
    int bitbuf;
    int left;
    short *next;

    bitbuf = s->bitbuf;
    left = s->bitcnt;
    code = first = index = 0;
    len = 1;
    next = h->count + 1;
    while (1) {
        while (left--) {
            code |= bitbuf & 1;
            bitbuf >>= 1;
            count = *next++;
            if (code - count < first) {
                s->bitbuf = bitbuf;
                s->bitcnt = (s->bitcnt - len) & 7;
                return h->symbol[index + (code - first)];
            }
            index += count;
            first += count;
            first <<= 1;
            code <<= 1;
            len++;
        }
        left = (MAXBITS+1) - len;
        if (left == 0)
            break;
        if (s->incnt == s->inlen) { s->error = 2; return -10; }
        bitbuf = s->in[s->incnt++];
        if (left > 8)
            left = 8;
    }
    return -10;
}

local int construct(struct huffman *h, const short *length, int n)
{
    int symbol;
    int len;
    int left;
    short offs[MAXBITS+1];

    for (len = 0; len <= MAXBITS; len++)
        h->count[len] = 0;
    for (symbol = 0; symbol < n; symbol++)
        (h->count[length[symbol]])++;
    if (h->count[0] == n)
        return 0;

    left = 1;
    for (len = 1; len <= MAXBITS; len++) {
        left <<= 1;
        left -= h->count[len];
        if (left < 0)
            return left;
    }

    offs[1] = 0;
    for (len = 1; len < MAXBITS; len++)
        offs[len + 1] = offs[len] + h->count[len];

    for (symbol = 0; symbol < n; symbol++)
        if (length[symbol] != 0)
            h->symbol[offs[length[symbol]]++] = symbol;

    return left;
}

local void fasttable(const struct huffman *h, int *tab) {
 int len;int i;int k;int rev;int v;int j;int first;int index;
 for(i=0;i<512;i++)tab[i]=-1;
 first=0;index=0;
 for(len=1;len<=MAXBITS;len++){
  if(len<=9)for(i=0;i<h->count[len];i++){
   v=first+i;rev=0;for(j=0;j<len;j++){rev=(rev<<1)|(v&1);v=v>>1;}
   for(k=rev;k<512;k+=1<<len)tab[k]=(h->symbol[index+i]<<4)|len;
  }
  index+=h->count[len];first=(first+h->count[len])<<1;
 }
}
local int fastdecode(struct state *s,const struct huffman *h,int *tab){
 int val;int avail;int entry;int n;
 val=s->bitbuf;avail=s->bitcnt;
 if(s->incnt<s->inlen){val|=((int)s->in[s->incnt])<<avail;avail+=8;}
 entry=tab[val&511];
 if(entry>=0){n=entry&15;if(n<=avail){if(n>s->bitcnt){s->incnt++;s->bitcnt+=8;}else val=s->bitbuf;s->bitbuf=val>>n;s->bitcnt-=n;return entry>>4;}}
 return decode(s,h);
}

local int codes(struct state *s,
                const struct huffman *lencode,
                const struct huffman *distcode)
{
    int lfast[512]; int dfast[512];
    int symbol;
    int len;
    unsigned dist;
    static const short lens[29] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static const short lext[29] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static const short dists[30] = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577};
    static const short dext[30] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
        12, 12, 13, 13};

    fasttable(lencode,lfast);fasttable(distcode,dfast);

    do {
        symbol = fastdecode(s, lencode,lfast);
        if (symbol < 0)
            return symbol;
        if (symbol < 256) {

            if (s->out != NIL) {
                if (s->outcnt == s->outlen)
                    return 1;
                s->out[s->outcnt] = symbol;
            }
            s->outcnt++;
        }
        else if (symbol > 256) {

            symbol -= 257;
            if (symbol >= 29)
                return -10;
            len = lens[symbol] + bits(s, lext[symbol]);

            symbol = fastdecode(s, distcode,dfast);
            if (symbol < 0 || symbol >= 30)
                return -10;
            dist = dists[symbol] + bits(s, dext[symbol]);
            if (s->error) return 2;
#ifndef INFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR
            if (dist > s->outcnt)
                return -11;
#endif

            if (s->out != NIL) {
                if (s->outcnt + len > s->outlen)
                    return 1;
                unsigned char *to=s->out+s->outcnt;
                unsigned char *from=to-dist;
                s->outcnt+=len;
                while(len--){*to=*from;to++;from++;}
            }
            else
                s->outcnt += len;
        }
    } while (symbol != 256);

    return 0;
}

local int fixed(struct state *s)
{
    static int virgin = 1;
    static short lencnt[MAXBITS+1], lensym[FIXLCODES];
    static short distcnt[MAXBITS+1], distsym[MAXDCODES];
    static struct huffman lencode, distcode;

    if (virgin) {
        int symbol;
        short lengths[FIXLCODES];

        lencode.count = lencnt;
        lencode.symbol = lensym;
        distcode.count = distcnt;
        distcode.symbol = distsym;

        for (symbol = 0; symbol < 144; symbol++)
            lengths[symbol] = 8;
        for (; symbol < 256; symbol++)
            lengths[symbol] = 9;
        for (; symbol < 280; symbol++)
            lengths[symbol] = 7;
        for (; symbol < FIXLCODES; symbol++)
            lengths[symbol] = 8;
        construct(&lencode, lengths, FIXLCODES);

        for (symbol = 0; symbol < MAXDCODES; symbol++)
            lengths[symbol] = 5;
        construct(&distcode, lengths, MAXDCODES);

        virgin = 0;
    }

    return codes(s, &lencode, &distcode);
}

local int dynamic(struct state *s)
{
    int nlen, ndist, ncode;
    int index;
    int err;
    short lengths[MAXCODES];
    short lencnt[MAXBITS+1], lensym[MAXLCODES];
    short distcnt[MAXBITS+1], distsym[MAXDCODES];
    struct huffman lencode, distcode;
    static const short order[19] =
        {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    lencode.count = lencnt;
    lencode.symbol = lensym;
    distcode.count = distcnt;
    distcode.symbol = distsym;

    nlen = bits(s, 5) + 257;
    ndist = bits(s, 5) + 1;
    ncode = bits(s, 4) + 4;
    if (s->error) return 2;
    if (nlen > MAXLCODES || ndist > MAXDCODES)
        return -3;

    for (index = 0; index < ncode; index++)
        lengths[order[index]] = bits(s, 3);
    for (; index < 19; index++)
        lengths[order[index]] = 0;

    if (s->error) return 2;
    err = construct(&lencode, lengths, 19);
    if (err != 0)
        return -4;

    index = 0;
    while (index < nlen + ndist) {
        int symbol;
        int len;

        symbol = decode(s, &lencode);
        if (symbol < 0)
            return symbol;
        if (symbol < 16)
            lengths[index++] = symbol;
        else {
            len = 0;
            if (symbol == 16) {
                if (index == 0)
                    return -5;
                len = lengths[index - 1];
                symbol = 3 + bits(s, 2);
            }
            else if (symbol == 17)
                symbol = 3 + bits(s, 3);
            else
                symbol = 11 + bits(s, 7);
            if (s->error) return 2;
            if (index + symbol > nlen + ndist)
                return -6;
            while (symbol--)
                lengths[index++] = len;
        }
    }

    if (lengths[256] == 0)
        return -9;

    err = construct(&lencode, lengths, nlen);
    if (err && (err < 0 || nlen != lencode.count[0] + lencode.count[1]))
        return -7;

    err = construct(&distcode, lengths + nlen, ndist);
    if (err && (err < 0 || ndist != distcode.count[0] + distcode.count[1]))
        return -8;

    return codes(s, &lencode, &distcode);
}

static int puff(unsigned char *dest,
         unsigned long *destlen,
         const unsigned char *source,
         unsigned long *sourcelen)
{
    struct state s;
    int last, type;
    int err;

    s.out = dest;
    s.outlen = *destlen;
    s.outcnt = 0;

    s.in = source;
    s.inlen = *sourcelen;
    s.incnt = 0;
    s.bitbuf = 0;
    s.bitcnt = 0;
    s.error = 0;

    {

        do {
            last = bits(&s, 1);
            type = bits(&s, 2);
            if (s.error) { err = 2; break; }
            err = type == 0 ?
                    stored(&s) :
                    (type == 1 ?
                        fixed(&s) :
                        (type == 2 ?
                            dynamic(&s) :
                            -1));
            if (err != 0)
                break;
        } while (!last);
    }

    if (err <= 0) {
        *destlen = s.outcnt;
        *sourcelen = s.incnt;
    }
    return err;
}

#ifndef UNISA_RUNTIME_STATE
#define UNISA_RUNTIME_STATE static
#endif
UNISA_RUNTIME_STATE uint32_t ctab[4][256];
static int crc_init(void){int i;int j;uint32_t c;for(i=0;i<256;i++){c=(uint32_t)i;for(j=0;j<8;j++){if(c&1)c=(c>>1)^3988292384U;else c=c>>1;}ctab[0][i]=c;}for(i=0;i<256;i++){c=ctab[0][i];for(j=1;j<4;j++){c=(c>>8)^ctab[0][c&255];ctab[j][i]=c;}}return 0;}
static uint32_t crc(char *p,int n){uint32_t c;uint32_t x;int i;c=4294967295U;i=0;
 while(i+4<=n){x=c^((uint32_t)(p[i]&255)|((uint32_t)(p[i+1]&255)<<8)|((uint32_t)(p[i+2]&255)<<16)|((uint32_t)(p[i+3]&255)<<24));c=ctab[3][x&255]^ctab[2][(x>>8)&255]^ctab[1][(x>>16)&255]^ctab[0][(x>>24)&255];i+=4;}
 while(i<n){c=(c>>8)^ctab[0][(c^(p[i]&255))&255];i++;}return c^4294967295U;}
/* Exact extent wrapper: never executes a partially decoded model. */
static int unisa_inflate(unsigned char *out, unsigned long outlen,
 const unsigned char *in, unsigned long inlen) {
 if (!out || !in) return -12;
 unsigned long n = outlen, m = inlen;
 int rc = puff(out, &n, in, &m);
 if (rc) return rc;
 if (n != outlen || m != inlen) return 3;
 return 0;
}
#undef local
#undef NIL
#undef MAXBITS
#undef MAXLCODES
#undef MAXDCODES
#undef MAXCODES
#undef FIXLCODES
