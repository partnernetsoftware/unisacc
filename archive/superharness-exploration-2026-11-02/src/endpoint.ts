import { mkdirSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";

export interface HarnessEndpoint {
  url: string;
  token: string;
}

export function endpointPath(): string {
  return process.env.SUPERHARNESS_ENDPOINT ?? join(import.meta.dir, "..", "endpoint.env");
}

export function writeEndpoint(endpoint: HarnessEndpoint): void {
  const path = endpointPath();
  mkdirSync(dirname(path), { recursive: true });
  writeFileSync(path, `SUPERHARNESS_URL=${endpoint.url}\nSUPERHARNESS_TOKEN=${endpoint.token}\n`, { mode: 0o600 });
}

export function readEndpoint(): HarnessEndpoint | null {
  let text: string;
  try {
    text = readFileSync(endpointPath(), "utf8");
  } catch {
    return null;
  }
  const values = new Map<string, string>();
  for (const line of text.split("\n")) {
    const at = line.indexOf("=");
    if (at > 0) values.set(line.slice(0, at), line.slice(at + 1));
  }
  const url = values.get("SUPERHARNESS_URL");
  const token = values.get("SUPERHARNESS_TOKEN");
  return url && token ? { url, token } : null;
}

export function removeEndpoint(): void {
  rmSync(endpointPath(), { force: true });
}
