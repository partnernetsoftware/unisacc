export function bindAudioPCM(memory, contextForHandle) {
  const endTimes = new WeakMap();

  // ptr is a physical Wasm address produced by the base's ux_span.
  function copy(context, ptr, frames, channels, rate) {
    if (ptr < 0n || ptr % 4n || frames < 1n || frames > 48000n ||
        (channels !== 1n && channels !== 2n) || rate < 8000n || rate > 192000n) return;
    const bytes = frames * channels * 4n;
    const buffer = memory.buffer;
    if (ptr + bytes > BigInt(buffer.byteLength)) return;
    const input = new DataView(buffer, Number(ptr), Number(bytes));
    const count = Number(frames);
    const width = Number(channels);
    const pcm = context.createBuffer(width, count, Number(rate));
    for (let channel = 0; channel < width; channel++) {
      const output = pcm.getChannelData(channel);
      for (let frame = 0; frame < count; frame++) {
        const sample = input.getFloat32((frame * width + channel) * 4, true);
        if (!Number.isFinite(sample)) return;
        output[frame] = sample;
      }
    }
    return pcm;
  }

  return {
    buffer(handle, ptr, frames, channels, rate) {
      const context = contextForHandle(handle);
      if (!context || context.state === "closed") return;
      return copy(context, ptr, frames, channels, rate);
    },

    write(handle, ptr, frames, channels, rate) {
      const context = contextForHandle(handle);
      if (!context || context.state === "closed") return -1n;
      const pcm = copy(context, ptr, frames, channels, rate);
      if (!pcm) return -1n;
      const source = context.createBufferSource();
      source.buffer = pcm;
      source.connect(context.destination);
      const when = Math.max(context.currentTime, endTimes.get(context) ?? 0);
      source.start(when);
      endTimes.set(context, when + pcm.duration);
      return frames;
    },
  };
}
