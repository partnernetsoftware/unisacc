import type { RouterPromptOptions, RouterReply, RouterRpc } from "./router-rpc";

export interface OmpRpcOptions {
  model: string;
  cwd: string;
  env?: Record<string, string>;
}

interface PendingResponse {
  resolve(value: unknown): void;
  reject(error: Error): void;
}

interface PromptResult {
  status: string;
  error?: { message?: string };
}

function withTimeout<T>(promise: Promise<T>, timeoutMs: number, label: string): Promise<T> {
  let timer: ReturnType<typeof setTimeout> | undefined;
  const timeout = new Promise<never>((_resolve, reject) => {
    timer = setTimeout(() => reject(new Error(`${label} timed out after ${String(timeoutMs)}ms`)), timeoutMs);
  });
  return Promise.race([promise, timeout]).finally(() => clearTimeout(timer));
}

/**
 * D3b transport: one foreground `omp --mode rpc` process, JSONL on stdin/stdout.
 * The harness owns process lifecycle; this class only speaks the protocol.
 */
export class OmpRpcClient implements RouterRpc {
  readonly model: string;
  readonly cwd: string;
  readonly env: Record<string, string>;
  pid = 0;
  pgid = 0;
  #child: Bun.Subprocess | undefined;
  #stopped = false;
  #sequence = 0;
  #buffer = "";
  #responses = new Map<string, PendingResponse>();
  #prompts = new Map<string, PendingResponse>();

  constructor(options: OmpRpcOptions) {
    this.model = options.model;
    this.cwd = options.cwd;
    this.env = options.env ?? {};
  }

  async start(): Promise<void> {
    const extension = new URL("../extension.ts", import.meta.url).pathname;
    this.#child = Bun.spawn(
      [
        "omp",
        "--mode",
        "rpc",
        "--model",
        this.model,
        "--max-time",
        "1200",
        "--no-session",
        "--auto-approve",
        "--no-ui",
        "--tools",
        "harness_read,harness_route",
        "-e",
        extension,
      ],
      {
        cwd: this.cwd,
        env: { ...process.env, ...this.env },
        detached: true,
        stdio: ["pipe", "pipe", "pipe"],
      },
    );
    this.pid = this.#child.pid;
    this.pgid = this.#child.pid;
    void this.#readLoop();
  }

  async #readLoop(): Promise<void> {
    const stdout = this.#child?.stdout;
    if (!stdout) return;
    const reader = stdout.getReader();
    const decoder = new TextDecoder();
    try {
      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        this.#buffer += decoder.decode(value, { stream: true });
        let newline = this.#buffer.indexOf("\n");
        while (newline >= 0) {
          const line = this.#buffer.slice(0, newline);
          this.#buffer = this.#buffer.slice(newline + 1);
          if (line.trim() !== "") this.#handleLine(line);
          newline = this.#buffer.indexOf("\n");
        }
      }
    } catch (error) {
      this.#failAll(error instanceof Error ? error : new Error(String(error)));
    }
  }

  #handleLine(line: string): void {
    let frame: unknown;
    try {
      frame = JSON.parse(line);
    } catch {
      return;
    }
    if (!frame || typeof frame !== "object") return;
    const record = frame as Record<string, unknown>;
    const id = typeof record.id === "string" ? record.id : undefined;
    if (record.type === "response" && id) {
      const waiter = this.#responses.get(id);
      if (waiter) {
        this.#responses.delete(id);
        waiter.resolve(record);
      }
      return;
    }
    if (record.type === "prompt_result" && id) {
      const waiter = this.#prompts.get(id);
      if (waiter) {
        this.#prompts.delete(id);
        waiter.resolve(record);
      }
    }
  }

  #failAll(error: Error): void {
    for (const waiter of this.#responses.values()) waiter.reject(error);
    for (const waiter of this.#prompts.values()) waiter.reject(error);
    this.#responses.clear();
    this.#prompts.clear();
  }

  #write(frame: object): void {
    const child = this.#child;
    if (!child || this.#stopped) throw new Error("router rpc is not running");
    child.stdin.write(`${JSON.stringify(frame)}\n`);
  }

  #nextId(prefix: string): string {
    this.#sequence++;
    return `${prefix}${String(this.#sequence)}`;
  }

  async #request(command: object, timeoutMs = 10_000): Promise<Record<string, unknown>> {
    const id = this.#nextId("c");
    const response = new Promise<unknown>((resolve, reject) => {
      this.#responses.set(id, { resolve, reject });
    });
    this.#write({ ...command, id });
    const frame = (await withTimeout(response, timeoutMs, "router rpc command")) as Record<string, unknown>;
    if (frame.success === false) throw new Error(`router rpc command failed: ${JSON.stringify(frame.error ?? frame)}`);
    return frame;
  }

  async newSession(): Promise<void> {
    await this.#request({ type: "new_session" });
  }

  async #lastAssistantText(): Promise<string> {
    const frame = await this.#request({ type: "get_last_assistant_text" });
    const data = frame.data as { text?: string | null } | undefined;
    return typeof data?.text === "string" ? data.text : "";
  }

  async #tokenTotal(): Promise<number> {
    const frame = await this.#request({ type: "get_session_stats" });
    const data = frame.data as { tokens?: { total?: number } } | undefined;
    return typeof data?.tokens?.total === "number" ? data.tokens.total : 0;
  }

  async prompt(message: string, options: RouterPromptOptions): Promise<RouterReply> {
    const baseline = await this.#tokenTotal();
    const id = this.#nextId("p");
    const result = new Promise<unknown>((resolve, reject) => {
      this.#prompts.set(id, { resolve, reject });
    });
    this.#write({ id, type: "prompt", message });
    let tokenExceeded = false;
    const monitor = setInterval(() => {
      void (async () => {
        if (tokenExceeded || this.#stopped) return;
        try {
          const total = await this.#tokenTotal();
          if (options.maxTokens !== undefined && total - baseline > options.maxTokens) {
            tokenExceeded = true;
            this.#write({ id: this.#nextId("abort"), type: "abort" });
          }
        } catch {
          // The process may be exiting; the prompt waiter owns the terminal error.
        }
      })();
    }, 250);
    let frame: Record<string, unknown>;
    try {
      frame = (await result) as Record<string, unknown>;
    } finally {
      clearInterval(monitor);
    }
    const after = await this.#tokenTotal().catch(() => baseline);
    const tokensUsed = Math.max(0, after - baseline);
    const promptResult = frame as unknown as PromptResult;
    if (!tokenExceeded && promptResult.status === "error") {
      throw new Error(promptResult.error?.message ?? "router prompt failed");
    }
    const text = await this.#lastAssistantText().catch(() => "");
    return { text, tokensUsed };
  }

  async stop(): Promise<void> {
    this.#stopped = true;
    this.#failAll(new Error("router rpc stopped"));
    const child = this.#child;
    if (!child) return;
    try {
      child.stdin.end();
    } catch {
      // Already closed.
    }
    if (this.pgid > 0) {
      try {
        process.kill(-this.pgid, "SIGTERM");
      } catch {
        // Already exited.
      }
    }
    await Promise.race([child.exited, Bun.sleep(1_000)]);
    if (child.exitCode === null && this.pgid > 0) {
      try {
        process.kill(-this.pgid, "SIGKILL");
      } catch {
        // Already exited.
      }
    }
  }
}
