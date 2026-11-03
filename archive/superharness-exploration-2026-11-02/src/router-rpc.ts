export interface RouterPromptOptions {
  timeoutMs: number;
  /** Harness-side cap; the real client aborts the turn once stats exceed it. */
  maxTokens?: number;
}

export interface RouterReply {
  text: string;
  tokensUsed: number;
}

/**
 * The Router transport seam. D3a ships a fake implementation in tests; D3b
 * supplies the real omp --mode rpc JSONL client. The harness owns the process
 * and lifecycle; this interface only sends a logical turn.
 */
export interface RouterRpc {
  /** Optional process lifecycle; a resident real client starts/stops itself. */
  start?(): Promise<void>;
  stop?(): Promise<void>;
  /** Clear conversation state while keeping the resident process alive. */
  newSession(): Promise<void>;
  prompt(message: string, options: RouterPromptOptions): Promise<RouterReply>;
}

export class RouterTimeoutError extends Error {
  constructor(timeoutMs: number) {
    super(`router turn timed out after ${String(timeoutMs)}ms`);
    this.name = "RouterTimeoutError";
  }
}
