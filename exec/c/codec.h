int cd_inflate(int, int, int, int, int);
int cd_puff_type(int);
int cd_bits_err(int);
int cd_len_rep(int, int, int *, int *, int *);
int cd_len_room(int, int, int, int);
int cd_block_pick(int);
int cd_puff_more(int, int);
int cd_puff_keep(int);
int cd_stored_byte(int, int, int);
int cd_room(unsigned long long, unsigned long long, unsigned long long, int);
int cd_bits_more(int, int);
int cd_bits_end(unsigned long long, unsigned long long);
int cd_bits_join(unsigned long long, int, int, unsigned long long *);
int cd_bits_add(int, int, int *);
int cd_bits_keep(unsigned long long, int, int *);
int cd_bits_left(int, int, int *);
int cd_bits_mask(unsigned long long, int, int *);
int cd_len_step(int, int, int *, int *);
int cd_in_byte(const unsigned char *, unsigned long long, unsigned long long, int *, unsigned long long *);
int cd_len_put(short *, int, int);
int cd_out_byte(unsigned char *, unsigned long long, unsigned long long, int, unsigned long long *);
int cd_span_byte(unsigned char *, const unsigned char *);
unsigned long long cd_crc_step(unsigned long long);
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
    while (cd_bits_more(s->bitcnt, need) == 1) {
        unsigned long long joined = 0;
        int added = 0;
        {
            int byte = 0;
            unsigned long long next = 0;
            int gotb = cd_in_byte(s->in, s->incnt, s->inlen, &byte, &next);
            if (gotb == 2) { s->error = 2; return 0; }
            if (gotb != 0) {
                if (s->incnt == s->inlen) { s->error = 2; return 0; }
                byte = s->in[s->incnt];
                next = s->incnt + 1;
            }
            if (cd_bits_join((unsigned long long)val, byte, s->bitcnt, &joined) == 0)
                val = (long)joined;
            else
                val |= (long)byte << s->bitcnt;
            s->incnt = next;
        }
        if (cd_bits_add(s->bitcnt, 8, &added) == 0) s->bitcnt = added;
        else s->bitcnt += 8;
    }

    {
        int kept = 0, left = 0, masked = 0;
        if (cd_bits_keep((unsigned long long)val, need, &kept) == 0) s->bitbuf = kept;
        else s->bitbuf = (int)(val >> need);
        if (cd_bits_left(s->bitcnt, need, &left) == 0) s->bitcnt = left;
        else s->bitcnt -= need;
        if (cd_bits_mask((unsigned long long)val, need, &masked) == 0) return masked;
    }
    return (int)(val & ((1L << need) - 1));
}

local int stored(struct state *s)
{
    unsigned len;

    s->bitbuf = 0;
    s->bitcnt = 0;

    if (cd_room(s->incnt, 4, s->inlen, 2) == 2)
        return 2;
    {
        int b = 0;
        unsigned long long next = 0;
        if (cd_in_byte(s->in, s->incnt, s->inlen, &b, &next) == 0) {
            len = (unsigned)b;
            s->incnt = next;
        } else
            len = s->in[s->incnt++];
        if (cd_in_byte(s->in, s->incnt, s->inlen, &b, &next) == 0) {
            len |= (unsigned)b << 8;
            s->incnt = next;
        } else
            len |= s->in[s->incnt++] << 8;
        {
            int chk;
            if (cd_in_byte(s->in, s->incnt, s->inlen, &b, &next) != 0) {
                b = s->in[s->incnt];
                next = s->incnt + 1;
            }
            chk = cd_stored_byte((int)len, 0, b);
            s->incnt = next;
            if (chk == (0 - 2))
                return (0 - 2);
            if (cd_in_byte(s->in, s->incnt, s->inlen, &b, &next) != 0) {
                b = s->in[s->incnt];
                next = s->incnt + 1;
            }
            chk = cd_stored_byte((int)len, 1, b);
            s->incnt = next;
            if (chk == (0 - 2))
                return (0 - 2);
        }
    }

    if (cd_room(s->incnt, len, s->inlen, 2) == 2)
        return 2;
    if (s->out != NIL) {
        if (cd_room(s->outcnt, len, s->outlen, 1) == 1)
            return 1;
        while (len--) {
            int b = 0;
            unsigned long long next = 0;
            unsigned long long onext = 0;
            if (cd_in_byte(s->in, s->incnt, s->inlen, &b, &next) != 0) {
                b = s->in[s->incnt];
                next = s->incnt + 1;
            }
            if (cd_out_byte(s->out, s->outcnt, s->outlen, b, &onext) == 0)
                s->outcnt = onext;
            else
                s->out[s->outcnt++] = (unsigned char)b;
            s->incnt = next;
        }
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
        {
            int b = 0;
            unsigned long long next = 0;
            int gotb = cd_in_byte(s->in, s->incnt, s->inlen, &b, &next);
            if (gotb == 2) { s->error = 2; return -10; }
            if (gotb != 0) {
                if (s->incnt == s->inlen) { s->error = 2; return -10; }
                b = s->in[s->incnt];
                next = s->incnt + 1;
            }
            bitbuf = b;
            s->incnt = next;
        }
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
 if(s->incnt<s->inlen){int b=0;unsigned long long next=0;if(cd_in_byte(s->in,s->incnt,s->inlen,&b,&next)==0)val|=b<<avail;else val|=((int)s->in[s->incnt])<<avail;avail+=8;}
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
                unsigned long long onext = 0;
                int put = cd_out_byte(s->out, s->outcnt, s->outlen, symbol, &onext);
                if (put == 1)
                    return 1;
                if (put == 0)
                    s->outcnt = onext;
                else {
                    if (s->outcnt == s->outlen)
                        return 1;
                    s->out[s->outcnt] = symbol;
                    s->outcnt++;
                }
            }
            else
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
            if (cd_bits_err(s->error)) return 2;
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
                while(len--){if(cd_span_byte(to,from)!=0)*to=*from;to++;from++;}
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
            if (cd_len_put(lengths, symbol, 8) != 0) lengths[symbol] = 8;
        for (; symbol < 256; symbol++)
            if (cd_len_put(lengths, symbol, 9) != 0) lengths[symbol] = 9;
        for (; symbol < 280; symbol++)
            if (cd_len_put(lengths, symbol, 7) != 0) lengths[symbol] = 7;
        for (; symbol < FIXLCODES; symbol++)
            if (cd_len_put(lengths, symbol, 8) != 0) lengths[symbol] = 8;
        construct(&lencode, lengths, FIXLCODES);

        for (symbol = 0; symbol < MAXDCODES; symbol++)
            if (cd_len_put(lengths, symbol, 5) != 0) lengths[symbol] = 5;
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
    if (cd_bits_err(s->error)) return 2;
    if (nlen > MAXLCODES || ndist > MAXDCODES)
        return -3;

    for (index = 0; index < ncode; index++) {
        int v = bits(s, 3);
        if (cd_len_put(lengths, order[index], v) != 0) lengths[order[index]] = (short)v;
    }
    for (; index < 19; index++)
        if (cd_len_put(lengths, order[index], 0) != 0) lengths[order[index]] = 0;

    if (cd_bits_err(s->error)) return 2;
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
        if (symbol < 16) {
            if (cd_len_put(lengths, index, symbol) != 0) lengths[index] = (short)symbol;
            index++;
        }
        else {
            int bits_n = 7, base = 11, copy = 0, got, room;
            len = 0;
            got = cd_len_rep(symbol, index, &bits_n, &base, &copy);
            if (got == (0 - 5)) return (0 - 5);
            if (got == 0) {
                if (copy) len = lengths[index - 1];
                symbol = base + bits(s, bits_n);
            }
            if (cd_bits_err(s->error)) return 2;
            room = cd_len_room(index, symbol, nlen, ndist);
            if (room == (0 - 6)) return (0 - 6);
            {
                int step, ns = 0, ni = 0;
                while ((step = cd_len_step(symbol, index, &ns, &ni)) == 1) {
                    if (cd_len_put(lengths, index, len) != 0) lengths[index] = (short)len;
                    symbol = ns;
                    index = ni;
                }
            }
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
    int err = 0;

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
            int pick;
            last = bits(&s, 1);
            type = bits(&s, 2);
            if (cd_bits_err(s.error)) { err = 2; break; }
            pick = cd_block_pick(type);
            if (pick == 0) err = stored(&s);
            else if (pick == 1) err = fixed(&s);
            else if (pick == 2) err = dynamic(&s);
            else err = -1;
            if (cd_puff_more(err, last) != 1) break;
        } while (1);
    }

    if (cd_puff_keep(err)) {
        *destlen = s.outcnt;
        *sourcelen = s.incnt;
    }
    return err;
}

#ifndef UNISA_RUNTIME_STATE
#define UNISA_RUNTIME_STATE static
#endif
UNISA_RUNTIME_STATE uint32_t ctab[4][256];
static int crc_init(void){int i;int j;uint32_t c;for(i=0;i<256;i++){c=(uint32_t)i;for(j=0;j<8;j++)c=(uint32_t)cd_crc_step(c);ctab[0][i]=c;}for(i=0;i<256;i++){c=ctab[0][i];for(j=1;j<4;j++){c=(c>>8)^ctab[0][c&255];ctab[j][i]=c;}}return 0;}
static uint32_t crc(char *p,int n){uint32_t c;uint32_t x;int i;c=4294967295U;i=0;
 while(i+4<=n){x=c^((uint32_t)(p[i]&255)|((uint32_t)(p[i+1]&255)<<8)|((uint32_t)(p[i+2]&255)<<16)|((uint32_t)(p[i+3]&255)<<24));c=ctab[3][x&255]^ctab[2][(x>>8)&255]^ctab[1][(x>>16)&255]^ctab[0][(x>>24)&255];i+=4;}
 while(i<n){c=(c>>8)^ctab[0][(c^(p[i]&255))&255];i++;}return c^4294967295U;}
/* Exact extent wrapper: never executes a partially decoded model. */
static int unisa_inflate(unsigned char *out, unsigned long outlen,
 const unsigned char *in, unsigned long inlen) {
 if (!out || !in) return cd_inflate(out != 0, in != 0, 0, 1, 1);
 unsigned long n = outlen, m = inlen;
 int rc = puff(out, &n, in, &m);
 return cd_inflate(1, 1, rc, n == outlen, m == inlen);
}
#undef local
#undef NIL
#undef MAXBITS
#undef MAXLCODES
#undef MAXDCODES
#undef MAXCODES
#undef FIXLCODES
