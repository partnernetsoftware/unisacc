export function bindAudioAnalyser(audio, {memory, command, offset, capacity}, fftSize = 2048) {
  const analyser = audio.context.createAnalyser();
  analyser.fftSize = fftSize;
  const bins = analyser.frequencyBinCount;
  const base = command(0n, BigInt(offset), BigInt(bins), BigInt(capacity));
  if (base < 0) throw new RangeError("Audio analyser byte arena");
  const data = new Uint8Array(memory.buffer, base, bins);
  const source = audio.getOutput();
  source.connect(analyser);
  let active = true;

  function getFrequencyData() {
    if (!active) throw new DOMException("Audio analyser is disposed", "InvalidStateError");
    analyser.getByteFrequencyData(data);
    return data;
  }

  return {
    analyser,
    data,
    getFrequencyData,

    getAverageFrequency() {
      getFrequencyData();
      return command(1n, BigInt(offset), BigInt(bins), BigInt(capacity));
    },

    dispose() {
      if (!active) return false;
      try {
        source.disconnect(analyser);
      } catch (error) {
        if (!(error instanceof DOMException) || error.name !== "InvalidAccessError") throw error;
      }
      active = false;
      return true;
    },
  };
}
