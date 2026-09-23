/* UXE Asteroid game core — freestanding {game}.wasm (NO UJS VM linked).
 * Imports:
 *   env.host_*     — browser Host ABI
 *   env.eng_boot   — load sim image into gameEngine.wasm
 *   env.eng_sim_step — one UJS step via gameEngine.wasm (glue bridges memories)
 */
#include <stdint.h>
#include "sim_embed.h"

typedef uint8_t  u8;
typedef uint32_t u32;
typedef int32_t  i32;
typedef double   f64;

#define IMP __attribute__((import_module("env")))

IMP __attribute__((import_name("host_time")))
extern f64 host_time(void);
IMP __attribute__((import_name("host_input_read")))
extern i32 host_input_read(u32 ptr, u32 cap);
IMP __attribute__((import_name("host_frame_begin")))
extern void host_frame_begin(void);
IMP __attribute__((import_name("host_frame_present")))
extern void host_frame_present(void);
IMP __attribute__((import_name("host_gpu_submit")))
extern void host_gpu_submit(u32 ptr, u32 len);
IMP __attribute__((import_name("host_log")))
extern void host_log(u32 level, u32 ptr, u32 len);
IMP __attribute__((import_name("host_request_frame")))
extern void host_request_frame(void);

/* Bridge into gameEngine.wasm (implemented in page glue). */
IMP __attribute__((import_name("eng_boot")))
extern i32 eng_boot(u32 image_ptr, u32 image_len);
IMP __attribute__((import_name("eng_sim_step")))
extern i32 eng_sim_step(u32 state_ptr, i32 ix, i32 iy, f64 dt);

enum { N = 480 };
enum { INPUT_BYTES = 20 };
enum { PACKET_MAGIC = 0x50455855u };

typedef struct {
  f64 xs[N], ys[N], zs[N], vxs[N], vys[N], vzs[N], rs[N];
  f64 px, py, pz, score, alive;
} SimState;

typedef struct {
  f64 ujs_ms, draw_ms, fps, score;
  u32 n, alive, ready, pad;
} HudSnap;

static SimState st;
static HudSnap hud;
static f64 last_ms, last_hud, acc_ujs, acc_gpu;
static u32 acc_frames;
static u8 input_buf[INPUT_BYTES];
static u8 packet_buf[1 << 18];

static void log_lit(u32 level, const char *s) {
  u32 n = 0;
  while (s[n]) n++;
  host_log(level, (u32)(uintptr_t)s, n);
}

static void fresh_state(void) {
  u32 i;
  for (i = 0; i < N; i++) {
    f64 sx = (f64)((i * 13) % 37) - 18.0;
    f64 sy = (f64)((i * 7) % 25) - 12.0;
    if (sx > -3.5 && sx < 3.5 && sy > -3.5 && sy < 3.5) sx += 7.0;
    st.xs[i] = sx; st.ys[i] = sy; st.zs[i] = -40.0 - (f64)i * 1.55;
    st.vxs[i] = (f64)((i % 5) - 2) * 0.55;
    st.vys[i] = (f64)((i % 3) - 1) * 0.4;
    st.vzs[i] = 8.0 + (f64)(i % 11) * 0.35;
    st.rs[i] = 0.55 + (f64)(i % 5) * 0.22;
  }
  st.px = st.py = st.pz = 0.0;
  st.score = 0.0;
  st.alive = 1.0;
}

static void wr_u32(u8 *p, u32 v) {
  p[0] = (u8)v; p[1] = (u8)(v >> 8); p[2] = (u8)(v >> 16); p[3] = (u8)(v >> 24);
}
static void wr_f32(u8 *p, f64 v) {
  float f = (float)v;
  u32 u;
  __builtin_memcpy(&u, &f, 4);
  wr_u32(p, u);
}

static u32 encode_packet(void) {
  u32 o = 0, i;
  f64 eye0 = st.px * 0.15, eye1 = st.py * 0.15 + 2.8, eye2 = st.pz + 11.0;
  f64 tgt0 = st.px * 0.05, tgt1 = st.py * 0.05, tgt2 = st.pz - 18.0;
  u32 nbytes = 8 + 16 + 36 + 4
    + (12 + 4 + N * 12 + N * 4)
    + (12 + 4 + 12 + 4);
  if (nbytes > sizeof(packet_buf)) return 0;

  wr_u32(packet_buf + o, PACKET_MAGIC); o += 4;
  wr_u32(packet_buf + o, 1); o += 4;
  wr_f32(packet_buf + o, 0.02); o += 4;
  wr_f32(packet_buf + o, 0.024); o += 4;
  wr_f32(packet_buf + o, 0.04); o += 4;
  wr_f32(packet_buf + o, 1.0); o += 4;
  wr_f32(packet_buf + o, 1.0471975511965976); o += 4;
  wr_f32(packet_buf + o, 0.1); o += 4;
  wr_f32(packet_buf + o, 300.0); o += 4;
  wr_f32(packet_buf + o, eye0); o += 4;
  wr_f32(packet_buf + o, eye1); o += 4;
  wr_f32(packet_buf + o, eye2); o += 4;
  wr_f32(packet_buf + o, tgt0); o += 4;
  wr_f32(packet_buf + o, tgt1); o += 4;
  wr_f32(packet_buf + o, tgt2); o += 4;
  wr_u32(packet_buf + o, 2); o += 4;

  wr_f32(packet_buf + o, 0.55); o += 4;
  wr_f32(packet_buf + o, 0.58); o += 4;
  wr_f32(packet_buf + o, 0.62); o += 4;
  wr_u32(packet_buf + o, N); o += 4;
  for (i = 0; i < N; i++) {
    wr_f32(packet_buf + o, st.xs[i]); o += 4;
    wr_f32(packet_buf + o, st.ys[i]); o += 4;
    wr_f32(packet_buf + o, st.zs[i]); o += 4;
  }
  for (i = 0; i < N; i++) { wr_f32(packet_buf + o, st.rs[i]); o += 4; }

  wr_f32(packet_buf + o, 0.35); o += 4;
  wr_f32(packet_buf + o, 0.75); o += 4;
  wr_f32(packet_buf + o, 1.0); o += 4;
  wr_u32(packet_buf + o, 1); o += 4;
  wr_f32(packet_buf + o, st.px); o += 4;
  wr_f32(packet_buf + o, st.py); o += 4;
  wr_f32(packet_buf + o, st.pz); o += 4;
  wr_f32(packet_buf + o, 0.7); o += 4;
  return o;
}

static void decode_input(i32 *ix, i32 *iy, u32 *fire) {
  *ix = (i32)((u32)input_buf[8] | ((u32)input_buf[9] << 8) |
              ((u32)input_buf[10] << 16) | ((u32)input_buf[11] << 24));
  *iy = (i32)((u32)input_buf[12] | ((u32)input_buf[13] << 8) |
              ((u32)input_buf[14] << 16) | ((u32)input_buf[15] << 24));
  *fire = (u32)input_buf[16] | ((u32)input_buf[17] << 8) |
          ((u32)input_buf[18] << 16) | ((u32)input_buf[19] << 24);
}

__attribute__((export_name("game_init")))
u32 game_init(void) {
  if (eng_boot((u32)(uintptr_t)SIM_IMAGE, SIM_IMAGE_LEN) != 0) {
    log_lit(2, "eng_boot failed");
    return 0;
  }
  fresh_state();
  if (eng_sim_step((u32)(uintptr_t)&st, 0, 0, 0.016) != 0) {
    log_lit(2, "eng_sim_step warm failed");
    return 0;
  }
  last_ms = host_time();
  last_hud = last_ms;
  acc_ujs = acc_gpu = 0.0;
  acc_frames = 0;
  hud.ready = 1;
  hud.n = N;
  hud.alive = st.alive != 0.0;
  hud.score = st.score;
  log_lit(0, "asteroid.wasm init (engine separate)");
  host_request_frame();
  return 1;
}

__attribute__((export_name("game_frame")))
void game_frame(void) {
  f64 now, dt, t0, g0, ujs_ms, gpu_ms;
  i32 ix, iy, nIn;
  u32 fire, plen;

  host_frame_begin();
  now = host_time();
  dt = (now - last_ms) / 1000.0;
  if (dt > 0.05) dt = 0.05;
  last_ms = now;

  nIn = host_input_read((u32)(uintptr_t)input_buf, INPUT_BYTES);
  if (nIn != INPUT_BYTES) {
    log_lit(2, "host_input_read size");
    host_request_frame();
    return;
  }
  decode_input(&ix, &iy, &fire);

  if (fire && st.alive == 0.0) fresh_state();

  ujs_ms = 0.0;
  if (st.alive != 0.0) {
    t0 = host_time();
    if (eng_sim_step((u32)(uintptr_t)&st, ix, iy, dt) != 0)
      log_lit(2, "eng_sim_step err");
    ujs_ms = host_time() - t0;
  }

  g0 = host_time();
  plen = encode_packet();
  if (plen) host_gpu_submit((u32)(uintptr_t)packet_buf, plen);
  host_frame_present();
  gpu_ms = host_time() - g0;

  acc_ujs += ujs_ms;
  acc_gpu += gpu_ms;
  acc_frames += 1;
  if (now - last_hud >= 500.0 && acc_frames) {
    hud.ujs_ms = acc_ujs / (f64)acc_frames;
    hud.draw_ms = acc_gpu / (f64)acc_frames;
    hud.fps = ((f64)acc_frames * 1000.0) / (now - last_hud);
    hud.score = st.score;
    hud.n = N;
    hud.alive = st.alive != 0.0;
    hud.ready = 1;
    acc_ujs = acc_gpu = 0.0;
    acc_frames = 0;
    last_hud = now;
  }
  host_request_frame();
}

__attribute__((export_name("game_hud_ptr")))
u32 game_hud_ptr(void) { return (u32)(uintptr_t)&hud; }

__attribute__((export_name("game_hud_size")))
u32 game_hud_size(void) { return (u32)sizeof(HudSnap); }

__attribute__((export_name("game_state_n")))
u32 game_state_n(void) { return N; }
