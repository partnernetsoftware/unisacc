/**
 * UXE Live — WebRTC signaling Worker (new name: uxe-live).
 *
 * HTTP:
 *   GET /health → { ok, service }
 *   GET /room/:id  (Upgrade: websocket) → Durable Object room
 *
 * WS JSON messages (client ↔ peers via room fan-out):
 *   { type: "join", role: "host"|"viewer", name? }
 *   { type: "signal", to?: peerId, payload }  // SDP / ICE; omit to → broadcast
 *   { type: "ping" } → { type: "pong" }
 *
 * No Cloudflare Realtime App Secret here — that stays Workers secrets when SFU lands.
 */
import { DurableObject } from "cloudflare:workers";

const CORS = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Methods": "GET, OPTIONS",
  "Access-Control-Allow-Headers": "Content-Type, Upgrade, Connection",
};

function json(data, status = 200) {
  return new Response(JSON.stringify(data), {
    status,
    headers: { "content-type": "application/json; charset=utf-8", ...CORS },
  });
}

export default {
  async fetch(request, env) {
    if (request.method === "OPTIONS") {
      return new Response(null, { status: 204, headers: CORS });
    }

    const url = new URL(request.url);
    if (url.pathname === "/health" || url.pathname === "/") {
      return json({ ok: true, service: "uxe-live", v: 1 });
    }

    const m = url.pathname.match(/^\/room\/([a-zA-Z0-9_-]{1,64})$/);
    if (!m) return json({ ok: false, err: "not_found" }, 404);

    if (request.headers.get("Upgrade") !== "websocket") {
      return json({ ok: false, err: "websocket_required", room: m[1] }, 426);
    }

    const id = env.ROOMS.idFromName(m[1]);
    return env.ROOMS.get(id).fetch(request);
  },
};

export class LiveRoom extends DurableObject {
  constructor(ctx, env) {
    super(ctx, env);
    this.ctx.setWebSocketAutoResponse(
      new WebSocketRequestResponsePair("ping", "pong"),
    );
  }

  async fetch(request) {
    const pair = new WebSocketPair();
    const [client, server] = Object.values(pair);
    const peerId = crypto.randomUUID().slice(0, 8);
    this.ctx.acceptWebSocket(server);
    server.serializeAttachment({ peerId, role: "viewer", name: "" });

    server.send(JSON.stringify({
      type: "welcome",
      peerId,
      peers: this.peerList(server),
    }));

    return new Response(null, { status: 101, webSocket: client });
  }

  peerList(except) {
    const out = [];
    for (const ws of this.ctx.getWebSockets()) {
      if (ws === except) continue;
      const a = ws.deserializeAttachment() || {};
      out.push({ peerId: a.peerId, role: a.role || "viewer", name: a.name || "" });
    }
    return out;
  }

  broadcast(from, msg, toPeerId) {
    const raw = typeof msg === "string" ? msg : JSON.stringify(msg);
    for (const ws of this.ctx.getWebSockets()) {
      if (ws === from) continue;
      const a = ws.deserializeAttachment() || {};
      if (toPeerId && a.peerId !== toPeerId) continue;
      try { ws.send(raw); } catch { /* dropped */ }
    }
  }

  async webSocketMessage(ws, message) {
    if (typeof message !== "string") return;
    let msg;
    try { msg = JSON.parse(message); } catch {
      ws.send(JSON.stringify({ type: "err", err: "bad_json" }));
      return;
    }

    const att = ws.deserializeAttachment() || {};
    const peerId = att.peerId;

    if (msg.type === "join") {
      const role = msg.role === "host" ? "host" : "viewer";
      const name = String(msg.name || "").slice(0, 32);
      ws.serializeAttachment({ peerId, role, name });
      this.broadcast(ws, {
        type: "peer_join",
        peer: { peerId, role, name },
        peers: this.peerList(null),
      });
      ws.send(JSON.stringify({
        type: "joined",
        peerId,
        role,
        name,
        peers: this.peerList(ws),
      }));
      return;
    }

    if (msg.type === "signal") {
      this.broadcast(ws, {
        type: "signal",
        from: peerId,
        payload: msg.payload,
      }, msg.to || null);
      return;
    }

    if (msg.type === "ping") {
      ws.send(JSON.stringify({ type: "pong", t: Date.now() }));
      return;
    }

    ws.send(JSON.stringify({ type: "err", err: "unknown_type" }));
  }

  async webSocketClose(ws) {
    const att = ws.deserializeAttachment() || {};
    this.broadcast(ws, {
      type: "peer_leave",
      peerId: att.peerId,
      peers: this.peerList(ws),
    });
  }
}
