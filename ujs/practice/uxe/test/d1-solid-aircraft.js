// Hand-painted rear sprite. A period is transparent. Each glyph is one pixel.
const ink={
  K:"#141a24", D:"#252e3c", M:"#3e4a56", L:"#64707a", H:"#8e9796",
  G:"#295c66", C:"#429ba4", B:"#8ad5cb", A:"#943c2a", O:"#d36132", Y:"#ed9448",
  S:"#556575", T:"#a8b3b5", U:"#263344", F:"#a74433", R:"#f15b31", J:"#ffad4a", W:"#ffe598"
};
const wing=[
  "...................KKKKKKKKKKKKKKKKKKKK",
  "...............KKKKDDDDDDDDMMMMMMMMMMMK",
  "...........KKKKDDDDMMMLLLLLMMMMMMMMDDDK",
  ".......KKKKDDDDMMMMMLLLLLMMMMMMMDDDDDKK",
  "....KKKAAAODDMLLLLLLLMMMMMMMDDDDDDDKKK.",
  "..KAAOOOOYODDMMMMMMMMMMDDDDDDDDDKKK....",
  ".KAOOYYYOADDDDDDDDDDDDDDDDDDKKK.......",
  ".KAAAOOADDDDDDDKKKKKKKKKKKKK..........",
  "KAAADDDDKKKKKKKK.......................",
  ".KKKKKKK..............................."
];
const fin=[
  "...KAAK",
  "..KAOYOK",
  "..KAOYOK",
  "..KDOOOK",
  "..KDMOOK",
  ".KDDMAOK",
  ".KDMDAOK",
  "KDDMDDK.",
  "KDDDDK..",
  ".KKKK..."
];
const root=[
  "..KKKMMMMMK.",
  ".KMMMLLMMMMK",
  "KDDMMMLMMMMD",
  "KDDMMMMMMMDD",
  ".KDDMDDDDDDK",
  "..KKDDDDDDK.",
  "...KKKKKKK.."
];
const body=[
  "........KKKKKKKK........",
  "......KKGGGGGGGCKK......",
  ".....KGGCCCBBCBCCK......",
  "....KGGGGGCCCCGGGDK.....",
  "...KMMGGGGGGGGMMLDK.....",
  "..KDMMLLLLMLLLLLMMKK....",
  ".KDMLLHHLLMMMMMHLLMDK...",
  ".KDMMLLLMMMMMMMMMLMDK...",
  "KDDMMDMMMMMMMLLMMDMDDK..",
  "KDDMLLMMDDDDDMMLLMDDDK..",
  "KDDDMDDDDMMMMDDDDMDDDK..",
  "KDDDKKKKKKKKKKKKKDDDDK..",
  ".KDDKKMMMMKDKMMMMKKDK...",
  ".KDKKMDGDMKDKMDGDMKKDK..",
  ".KDKMDDGDDKDKMDDGDDKDK..",
  "..KKMMMMMMKKKMMMMMMKK...",
  "...KKKKKKK...KKKKKKK...."
];
const keel=[
  "....KKKKKKKKKKKKKKKK....",
  "..KUUUMMMMMMMMMMUUUUK..",
  ".KUMMSSTTTTTTTSSSMMUUK.",
  "KUMSSSSMMMMMMMMMSSSSMUK",
  "KUMMSMDDDDDDDDDDMSSMUK",
  ".KUUMMDDDDDDDDDDMMUUK.",
  ".KDDDMMSMDDDDMSMDDDK.",
  "..KDDDMMMMDDMMMMDDK..",
  "...KKDDDDDDDDDDKK...",
  "..KKUUDMMMMMMDUUKK..",
  "...KKKKKKKKKKKKKK..."
];
const jet=[
  ".KMMKK.",
  "KRYWJRK",
  "FRJWWRF",
  "FRJWWRF",
  ".RJWJR.",
  ".FRJJR.",
  ".FRJRF.",
  "..RJR..",
  "..FFR..",
  "...R...",
  "...F..."
];
const plate=document.createElement("canvas");
plate.width=96;
plate.height=50;
const draw=plate.getContext("2d");
function stamp(rows,x,y,mirror=false) {
  for(let row=0;row<rows.length;row++) for(let col=0;col<rows[row].length;col++) {
    const color=ink[rows[row][col]];
    if(!color) continue;
    draw.fillStyle=color;
    draw.fillRect(mirror?x-col:x+col,y+row,1,1);
  }
}

export function drawAircraft(buffer,x,y,pitch,roll,speed,tick) {
  draw.clearRect(0,0,96,50);
  stamp(wing,8,21);
  stamp(wing,87,21,true);
  stamp(wing,8,18);
  stamp(wing,87,18,true);
  stamp(root,28,18);
  stamp(root,67,18,true);
  const finOffset=Math.round(pitch/6);
  stamp(fin,26,9+finOffset);
  stamp(fin,69,9+finOffset,true);
  stamp(keel,36,25);
  stamp(body,37,10);
  stamp(["..KLLHHHLLK..",".KSMTTTTTMSK.","KUMSSMMSSMUK","KUDMMMMMMDDK"],
    43,25);
  const flicker=(tick%5===0?1:0)+(speed>18?2:0);
  stamp(jet.slice(0,8+flicker),39,28);
  stamp(jet.slice(0,8+(tick%5===2?1:0)+(speed>18?2:0)),50,28);
  buffer.save();
  buffer.translate(x,y);
  buffer.rotate(roll);
  buffer.imageSmoothingEnabled=false;
  buffer.drawImage(plate,-48,-26,96,55);
  buffer.restore();
}
