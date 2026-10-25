/* shim for miniz: zlib-compatible names as plain macros (used with
   -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES to dodge the "label defined twice" issue) */
#define z_stream mz_stream
#define deflateInit mz_deflateInit
#define deflate mz_deflate
#define deflateEnd mz_deflateEnd
#define deflateBound mz_deflateBound
#define inflateInit mz_inflateInit
#define inflate mz_inflate
#define inflateEnd mz_inflateEnd
#define compress mz_compress
#define compress2 mz_compress2
#define compressBound mz_compressBound
#define uncompress mz_uncompress
#define crc32 mz_crc32
#define adler32 mz_adler32
#define Z_OK MZ_OK
#define Z_STREAM_END MZ_STREAM_END
#define Z_BUF_ERROR MZ_BUF_ERROR
#define Z_NO_FLUSH MZ_NO_FLUSH
#define Z_FINISH MZ_FINISH
#define Z_SYNC_FLUSH MZ_SYNC_FLUSH
#define Z_BEST_COMPRESSION MZ_BEST_COMPRESSION
#define Z_DEFAULT_COMPRESSION MZ_DEFAULT_COMPRESSION
#define Z_DEFAULT_STRATEGY MZ_DEFAULT_STRATEGY
#define Z_DEFLATED MZ_DEFLATED
#define Z_DATA_ERROR MZ_DATA_ERROR
#define Z_MEM_ERROR MZ_MEM_ERROR
#define Z_NEED_DICT MZ_NEED_DICT
#define Z_VERSION_ERROR MZ_VERSION_ERROR
#define Z_PARAM_ERROR MZ_PARAM_ERROR
#define Z_STREAM_ERROR MZ_STREAM_ERROR
#define Z_NULL 0
#define ZLIB_VERSION MZ_VERSION
#define uLong mz_ulong
#define uInt unsigned int
#define Byte unsigned char
