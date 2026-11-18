export function windTuft() {
  const positions=[],normals=[];
  for(let i=0;i<8;i++) {
    const angle=i*2.39996,x=Math.cos(angle)*.31,z=Math.sin(angle)*.25;
    const h=.11+.05*(i%5),w=.037+.006*(i%3),tip=x*.75+.15;
    const a=[x-w,0,z],b=[x+w,0,z],c=[tip+w,h*.64,z*.91],
      d=[tip-w,h*.62,z*.91],e=[tip+.065,h,z*.86];
    positions.push(...a,...b,...c,...a,...c,...d,...d,...c,...e);
    for(let vertex=0;vertex<9;vertex++) normals.push(-.22,.89,.39);
  }
  return {positions:new Float32Array(positions),normals:new Float32Array(normals),vCount:positions.length/3};
}

export function buildWornDeck(height,span,back,front,recession,addMesh,instance) {
  const tones=[[],[],[],[],[]];
  const width=front-back;
  function face(vertices,ax,az,bx,bz,cx,cz,y) {
    for(const [x,z] of [[ax,az],[bx,bz],[cx,cz]]) vertices.push(x,y,z+recession(x));
  }
  function quad(vertices,x,z,right,far,y,chip) {
    face(vertices,x+chip,z,x,far-chip,right-chip,far,y);
    face(vertices,x+chip,z,right-chip,far,right,z+chip,y);
  }
  quad(tones[0],-span/2,back,span/2,front,height+.017,0);
  for(let row=0;row<13;row++) {
    const z=back+row*width/13,next=back+(row+1)*width/13;
    let x=-span/2,slot=0;
    while(x<span/2-.055) {
      const id=(row*119+slot*53)%101;
      const right=Math.min(span/2,x+.84+id*.008+(row%2===0 && slot===0?.39:0));
      const color=1+id%3;
      quad(tones[color],x+.023,z+.029,right-.023,next-.029,height+.025,.06+id*.0005);
      if(id%5===1 || id%7===3) {
        const mark=x+(right-x)*.32, edge=z+(next-z)*.50;
        const a=.08+(id%9)*.009, b=.065+(id%13)*.004;
        const flecks=[[-a,-b*.2],[-a*.55,b*.7],[a*.19,b],[a*.82,b*.1],[a*.34,-b*.91],[-a*.38,-b]];
        for(let j=0;j<flecks.length;j++) {
          const [px,pz]=flecks[j], [qx,qz]=flecks[(j+1)%flecks.length];
          face(tones[4],mark,edge,mark+px,edge+pz,mark+qx,edge+qz,height+.027);
        }
      }
      x=right;
      slot++;
    }
  }
  for(const [index,vertices] of tones.entries()) {
    const normals=new Float32Array(vertices.length);
    for(let i=1;i<normals.length;i+=3) normals[i]=1;
    const mesh=addMesh({positions:new Float32Array(vertices),normals,vCount:vertices.length/3});
    instance(["deckSeam","deckSlate","deckSun","deckShade","deckWear"][index],mesh,0,0,0,1);
  }
}
