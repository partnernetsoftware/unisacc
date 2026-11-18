// Hand-painted rear sprite. A period is transparent. Each glyph is one pixel.
const ink={
  K:"#141a24", D:"#252e3c", M:"#3e4a56", L:"#64707a", H:"#8e9796",
  G:"#295c66", C:"#429ba4", B:"#8ad5cb", A:"#943c2a", O:"#d36132", Y:"#ed9448",
  F:"#a74433", R:"#f15b31", J:"#ffad4a", W:"#ffe598"
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

export function drawAircraft(buffer,x,y,pitch,bank,speed,tick) {
  draw.clearRect(0,0,96,50);
  stamp(wing,8,18);
  stamp(wing,87,18,true);
  stamp(fin,26,9);
  stamp(fin,69,9,true);
  stamp(body,37,10);
  const flicker=(tick%5===0?1:0)+(speed>18?2:0);
  stamp(jet.slice(0,8+flicker),39,24);
  stamp(jet.slice(0,8+(tick%5===2?1:0)+(speed>18?2:0)),50,24);
  buffer.save();
  buffer.translate(Math.round(x),Math.round(y-pitch*.18));
  buffer.rotate(bank*Math.PI/180);
  buffer.imageSmoothingEnabled=false;
  buffer.drawImage(plate,-48,-23);
  buffer.restore();
}
