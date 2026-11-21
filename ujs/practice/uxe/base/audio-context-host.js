export function bindAudioContext(command) {
  let active;

  return {
    open(requested = 0) {
      if (!Number.isInteger(requested)) return -1n;
      const handle = command(1n, 0n, BigInt(requested));
      if (handle < 0n) return handle;

      let context;
      try {
        context = requested ? new AudioContext({ sampleRate: requested }) : new AudioContext();
        if (command(2n, handle, BigInt(context.sampleRate)) === 1n) {
          active = { handle, context };
          return handle;
        }
      } catch (error) {
        if (!(error instanceof DOMException)) {
          command(2n, handle, 0n);
          throw error;
        }
      }
      command(2n, handle, 0n);
      if (context) void context.close().catch(() => {});
      return -1n;
    },

    sampleRate(handle) {
      return command(5n, handle, 0n);
    },

    context(handle) {
      if (active?.handle === handle && command(5n, handle, 0n) >= 0n) return active.context;
      return undefined;
    },

    async close(handle) {
      if (active?.handle !== handle || command(3n, handle, 0n) !== 1n) return -1n;
      try {
        await active.context.close();
      } catch (error) {
        command(4n, handle, 0n);
        if (!(error instanceof DOMException)) throw error;
        return -1n;
      }
      active = undefined;
      return command(4n, handle, 1n);
    },
  };
}
