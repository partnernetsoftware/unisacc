import type { RouterPromptOptions, RouterReply, RouterRpc } from "../src/router-rpc";

export interface FakeReply {
  text: string;
  tokensUsed?: number;
  delayMs?: number;
}

/** Deterministic D3a fake: no omp process, no network, no DB access. */
export class FakeRouterRpc implements RouterRpc {
  readonly prompts: string[] = [];
  newSessionCalls = 0;
  #index = 0;

  constructor(private readonly replies: FakeReply[]) {}

  async newSession(): Promise<void> {
    this.newSessionCalls++;
  }

  async prompt(message: string, _options: RouterPromptOptions): Promise<RouterReply> {
    this.prompts.push(message);
    const reply = this.replies[this.#index++] ?? { text: '{"decision":"ignore","reason":"fake exhausted"}' };
    if (reply.delayMs !== undefined && reply.delayMs > 0) await Bun.sleep(reply.delayMs);
    return { text: reply.text, tokensUsed: reply.tokensUsed ?? 10 };
  }
}
