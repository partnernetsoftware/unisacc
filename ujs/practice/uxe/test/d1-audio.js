import {sound} from "./d1-audio-data.js";

function synth(context,destination,name,flat=false) {
  const [wave,start,end,gain,attack,duration]=sound[name];
  const now=context.currentTime;
  const oscillator=context.createOscillator(), envelope=context.createGain();
  oscillator.type=wave;
  oscillator.frequency.setValueAtTime(start,now);
  oscillator.frequency.linearRampToValueAtTime(end,now+duration/1000);
  envelope.gain.setValueAtTime(flat?gain/1000:0,now);
  if(!flat) {
    envelope.gain.linearRampToValueAtTime(gain/1000,now+attack/1000);
    envelope.gain.linearRampToValueAtTime(0,now+duration/1000);
  }
  oscillator.connect(envelope).connect(destination);
  oscillator.onended=()=>{ oscillator.disconnect(); envelope.disconnect(); };
  oscillator.start(now);
  oscillator.stop(now+duration/1000);
}

export function audioModel(query) {
  const events={gun:0,missile:0,explosion:0,lock:0};
  const [wave,base,slope,level,ramp]=sound.engine;
  let context,master,motor,muted=false,speed=12;
  function unlock() {
    if(!globalThis.AudioContext) return;
    if(!context) {
      context=new AudioContext();
      master=context.createGain();
      master.gain.value=muted?0:1;
      master.connect(context.destination);
      motor=context.createOscillator();
      const volume=context.createGain();
      motor.type=wave;
      motor.frequency.value=base+slope*speed;
      volume.gain.value=level/1000;
      motor.connect(volume).connect(master);
      motor.start();
    }
    if(context.state==="suspended") context.resume().catch(()=>{});
  }
  function engine(value) {
    if(speed===value) return;
    speed=value;
    if(motor) motor.frequency.setTargetAtTime(base+slope*value,context.currentTime,ramp/1000);
  }
  function toggle() {
    muted=!muted;
    if(master) master.gain.setTargetAtTime(muted?0:1,context.currentTime,.015);
    return muted;
  }
  function trigger(name) {
    if(query.has("no-lock-sound") && name==="lock") return;
    events[name]++;
    if(context?.state==="running" && !muted) synth(context,master,name);
  }
  async function sample(points) {
    const offline=new OfflineAudioContext(1,1024,8000);
    synth(offline,offline.destination,"lock",query.has("flat-envelope"));
    const buffer=(await offline.startRendering()).getChannelData(0);
    return [...Object.values(events),base+slope*speed,...points.map(index=>Math.round(buffer[index]*10000))];
  }
  return {unlock,engine,toggle,trigger,sample,counts:()=>Object.values(events)};
}
