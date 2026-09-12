// The "ATOM FRAMER" web UI, served from flash at "/".
// Adapted from the standalone exporter: adds a SEND -> DEVICE action that POSTs
// the packed 128x128 RGB565 bytes to POST /frame on this device.
#pragma once
#include <pgmspace.h>

static const char INDEX_HTML[] PROGMEM = R"=====(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ATOM FRAMER — 128×128 RGB565</title>
<style>
  :root{
    --bg:#0d0f0d; --panel:#15181500; --ink:#c8d4c0; --dim:#5e6b58;
    --line:#2a3128; --hot:#b6ff3a; --warn:#ff5b3a; --pad:#1b1f1a;
    --grid:#1a1e18;
  }
  *{box-sizing:border-box;margin:0;padding:0}
  html,body{height:100%}
  body{
    background:var(--bg);
    color:var(--ink);
    font-family:"DM Mono",ui-monospace,"Cascadia Mono",Menlo,monospace;
    display:flex;flex-direction:column;
    background-image:
      linear-gradient(var(--grid) 1px,transparent 1px),
      linear-gradient(90deg,var(--grid) 1px,transparent 1px);
    background-size:22px 22px;
    overflow:hidden;
  }
  header{
    padding:10px 16px;border-bottom:1px solid var(--line);
    display:flex;align-items:baseline;gap:14px;flex-wrap:wrap;
    background:rgba(13,15,13,.85);backdrop-filter:blur(2px);
  }
  header h1{font-size:15px;letter-spacing:.32em;font-weight:700;color:var(--hot)}
  header .sub{font-size:11px;color:var(--dim);letter-spacing:.15em}
  .wrap{flex:1;display:flex;min-height:0}
  .stage{
    flex:1;position:relative;overflow:hidden;
    background:repeating-conic-gradient(#101310 0% 25%,#0b0d0b 0% 50%) 0/24px 24px;
  }
  #view{position:absolute;inset:0;cursor:grab;width:100%;height:100%;display:block}
  #view.drag{cursor:grabbing}
  .hint{
    position:absolute;bottom:10px;left:12px;font-size:11px;color:var(--dim);
    letter-spacing:.1em;pointer-events:none;
    background:rgba(13,15,13,.6);padding:4px 8px;border:1px solid var(--line);
  }
  aside{
    width:300px;border-left:1px solid var(--line);
    background:rgba(13,15,13,.9);
    display:flex;flex-direction:column;padding:16px;gap:18px;overflow-y:auto;
  }
  .group{display:flex;flex-direction:column;gap:8px}
  .label{font-size:10px;letter-spacing:.28em;color:var(--dim);text-transform:uppercase}
  .preview-box{align-self:center;position:relative}
  #preview{
    width:192px;height:192px;image-rendering:pixelated;
    border:1px solid var(--hot);background:#000;display:block;
    box-shadow:0 0 0 4px var(--pad),0 0 24px rgba(182,255,58,.15);
  }
  .preview-cap{font-size:10px;color:var(--dim);text-align:center;margin-top:6px;letter-spacing:.2em}
  .row{display:flex;gap:8px}
  button,label.btn{
    flex:1;font:inherit;font-size:12px;letter-spacing:.12em;
    background:var(--pad);color:var(--ink);border:1px solid var(--line);
    padding:9px 8px;cursor:pointer;text-align:center;text-transform:uppercase;
    transition:.12s;user-select:none;
  }
  button:hover,label.btn:hover{border-color:var(--hot);color:var(--hot)}
  button.primary{background:var(--hot);color:#0d0f0d;border-color:var(--hot);font-weight:700}
  button.primary:hover{background:#c9ff5a}
  button:disabled{opacity:.35;cursor:not-allowed;border-color:var(--line);color:var(--dim)}
  input[type=file]{display:none}
  .meta{font-size:11px;line-height:1.7;color:var(--dim)}
  .meta b{color:var(--ink);font-weight:400}
  .scale-readout{font-size:11px;color:var(--hot);letter-spacing:.1em}
  .slider{display:flex;align-items:center;gap:10px}
  input[type=range]{flex:1;accent-color:var(--hot);background:transparent}
  .div{height:1px;background:var(--line);margin:2px 0}
  .status{font-size:11px;color:var(--dim);letter-spacing:.06em;min-height:14px}
  .status.ok{color:var(--hot)}
  .status.err{color:var(--warn)}
  .slotbar{display:flex;gap:8px}
  .slotbtn{position:relative;flex:1;padding:10px 0;font-weight:700}
  .slotbtn.active{background:var(--hot);color:#0d0f0d;border-color:var(--hot)}
  .slotbtn.filled::after{content:"";position:absolute;top:4px;right:4px;width:6px;height:6px;
    border-radius:50%;background:var(--hot)}
  .slotbtn.active.filled::after{background:#0d0f0d}
  .gline{display:flex;align-items:center;gap:8px}
  .gline .gname{flex:0 0 96px;font-size:10px;letter-spacing:.1em;color:var(--dim);text-transform:uppercase}
  .gline .gurl{flex:1;min-width:0;font:inherit;font-size:11px;background:var(--pad);color:var(--ink);
    border:1px solid var(--line);padding:7px 8px}
  .gline .gurl:focus{outline:none;border-color:var(--hot)}
  .hint2{font-size:10px;color:var(--dim);letter-spacing:.04em}
  .peers{display:flex;flex-direction:column;gap:4px;font-size:11px}
  .peerrow{display:flex;gap:8px;align-items:baseline;cursor:pointer}
  .peerrow:hover .pn{text-decoration:underline}
  .peerrow .pn{color:var(--hot);font-weight:700}
  .peerrow .pip{color:var(--dim);font-size:10px}
</style>
</head>
<body>
<header>
  <h1>ATOM&nbsp;FRAMER</h1>
  <span class="sub">128×128 · 4 SLOTS · BUTTON GESTURES</span>
</header>

<div class="wrap">
  <div class="stage">
    <canvas id="view"></canvas>
    <div class="hint">DRAG TO PAN · WHEEL TO ZOOM · THE LIME BOX = DEVICE FRAME</div>
  </div>

  <aside>
    <div class="group">
      <span class="label">Source</span>
      <div class="row">
        <label class="btn">LOAD IMG<input type="file" id="fileImg" accept="image/*"></label>
        <label class="btn">LOAD .PNG<input type="file" id="filePng" accept="image/png"></label>
      </div>
    </div>

    <div class="div"></div>

    <div class="group">
      <span class="label">Slot (edit / send target)</span>
      <div class="slotbar" id="slotbar"></div>
    </div>

    <div class="div"></div>

    <div class="group preview-box">
      <span class="label" style="align-self:center">Device Preview</span>
      <canvas id="preview" width="128" height="128"></canvas>
      <div class="preview-cap">EXACTLY WHAT THE M5 SHOWS</div>
    </div>

    <div class="group">
      <span class="label">Zoom</span>
      <div class="slider">
        <input type="range" id="zoom" min="0.05" max="20" step="0.001" value="1" disabled>
        <span class="scale-readout" id="scaleOut">—</span>
      </div>
      <div class="row">
        <button id="fit" disabled>FIT</button>
        <button id="fill" disabled>FILL</button>
        <button id="one" disabled>1:1</button>
      </div>
    </div>

    <div class="div"></div>

    <div class="group">
      <span class="label">Commit</span>
      <button class="primary" id="sendDev" disabled>SEND → SLOT</button>
      <button id="showDev">SHOW THIS SLOT ON DEVICE</button>
      <button id="savePng" disabled>SAVE PNG + DATA</button>
      <div class="row">
        <button id="saveDat" disabled>EXPORT .DAT</button>
      </div>
      <div class="status" id="status">Load an image to begin.</div>
    </div>

    <div class="div"></div>

    <div class="group">
      <span class="label">Button gestures · slot <span id="gSlot">1</span></span>
      <div class="gline"><span class="gname">Short ·&lt;0.5s</span><input class="gurl" data-g="0" placeholder="1-4 or URL"></div>
      <div class="gline"><span class="gname">Long ·&gt;0.7s</span><input class="gurl" data-g="1" placeholder="1-4 or URL (empty = show IP)"></div>
      <div class="gline"><span class="gname">Double</span><input class="gurl" data-g="2" placeholder="1-4 or URL"></div>
      <button id="saveBtns">SAVE GESTURES</button>
      <div class="hint2">just <b>1–4</b> = show that slot here · <b>red_oak:1</b> = another unit by name</div>
    </div>

    <div class="div"></div>

    <div class="group">
      <span class="label">Network — I am <b id="devName" style="color:var(--hot)">…</b></span>
      <div id="peers" class="peers"><span class="hint2">discovering…</span></div>
      <div class="hint2">click a peer to drop <b>name:1</b> into the focused gesture field</div>
    </div>

    <div class="div"></div>

    <div class="meta">
      <b>OUT:</b> 128×128 px · RGB565 BE<br>
      <b>SLOTS:</b> 4 · <b>GESTURES:</b> short/long/double<br>
      <b>SIZE:</b> 32768 bytes (.dat)<br>
      <b>POST:</b> /frame?slot=N (multipart)
    </div>
  </aside>
</div>

<script>
"use strict";
const SIZE = 128;

// ---- state ----
const view = document.getElementById('view');
const vctx = view.getContext('2d');
const pv   = document.getElementById('preview');
const pctx = pv.getContext('2d', { willReadFrequently:true });
const status = document.getElementById('status');
const scaleOut = document.getElementById('scaleOut');
const zoomSlider = document.getElementById('zoom');

let img = null;          // source HTMLImageElement / ImageBitmap-ish (use Image)
let scale = 1;           // source px -> screen px
let ox = 0, oy = 0;      // top-left of source in screen coords (canvas space)
let dragging = false, lastX = 0, lastY = 0;
let frameRect = {x:0,y:0,s:0}; // lime box on screen (square, side s, top-left x,y)

function setStatus(msg, cls){ status.textContent = msg; status.className = 'status' + (cls?(' '+cls):''); }

// ---- canvas sizing ----
function resizeView(){
  const r = view.parentElement.getBoundingClientRect();
  view.width = r.width; view.height = r.height;
  computeFrame();
  draw();
}
window.addEventListener('resize', resizeView);

// The device frame is a fixed square centered in the stage.
function computeFrame(){
  const side = Math.min(view.width, view.height) * 0.6;
  frameRect = { x:(view.width-side)/2, y:(view.height-side)/2, s:side };
}

// ---- drawing ----
function draw(){
  vctx.clearRect(0,0,view.width,view.height);
  if(img){
    vctx.imageSmoothingEnabled = scale < 4; // crisp when zoomed way in
    vctx.drawImage(img, ox, oy, img.width*scale, img.height*scale);
  }
  // dim outside frame
  vctx.save();
  vctx.fillStyle = 'rgba(8,10,8,0.72)';
  vctx.beginPath();
  vctx.rect(0,0,view.width,view.height);
  vctx.rect(frameRect.x, frameRect.y, frameRect.s, frameRect.s);
  vctx.fill('evenodd');
  vctx.restore();
  // frame border
  vctx.strokeStyle = '#b6ff3a';
  vctx.lineWidth = 1.5;
  vctx.strokeRect(frameRect.x+0.5, frameRect.y+0.5, frameRect.s, frameRect.s);
  // corner ticks
  vctx.strokeStyle = '#b6ff3a';
  const t = 10;
  const cs = [[frameRect.x,frameRect.y,1,1],[frameRect.x+frameRect.s,frameRect.y,-1,1],
              [frameRect.x,frameRect.y+frameRect.s,1,-1],[frameRect.x+frameRect.s,frameRect.y+frameRect.s,-1,-1]];
  vctx.lineWidth = 2.5;
  for(const [cx,cy,dx,dy] of cs){
    vctx.beginPath();
    vctx.moveTo(cx, cy+dy*t); vctx.lineTo(cx,cy); vctx.lineTo(cx+dx*t,cy);
    vctx.stroke();
  }
  renderPreview();
}

// Render the 128x128 preview by sampling the source region under the frame.
function renderPreview(){
  if(!img) return;   // no source loaded — keep the device read-back shown
  pctx.clearRect(0,0,SIZE,SIZE);
  // source coords of frame's top-left and size
  const sx = (frameRect.x - ox) / scale;
  const sy = (frameRect.y - oy) / scale;
  const ss = frameRect.s / scale;
  pctx.imageSmoothingEnabled = true;
  pctx.fillStyle = '#000'; pctx.fillRect(0,0,SIZE,SIZE);
  pctx.drawImage(img, sx, sy, ss, ss, 0, 0, SIZE, SIZE);
}

// ---- interaction ----
view.addEventListener('mousedown', e=>{
  if(!img) return;
  dragging = true; lastX = e.clientX; lastY = e.clientY;
  view.classList.add('drag');
});
window.addEventListener('mouseup', ()=>{ dragging=false; view.classList.remove('drag'); });
window.addEventListener('mousemove', e=>{
  if(!dragging) return;
  ox += e.clientX - lastX; oy += e.clientY - lastY;
  lastX = e.clientX; lastY = e.clientY;
  draw();
});
view.addEventListener('wheel', e=>{
  if(!img) return;
  e.preventDefault();
  const rect = view.getBoundingClientRect();
  const mx = e.clientX - rect.left, my = e.clientY - rect.top;
  const factor = Math.exp(-e.deltaY * 0.0015);
  zoomAt(mx, my, scale*factor);
}, {passive:false});

function zoomAt(px, py, newScale){
  newScale = Math.min(20, Math.max(0.01, newScale));
  // keep point under cursor fixed
  const wx = (px - ox)/scale, wy = (py - oy)/scale;
  scale = newScale;
  ox = px - wx*scale; oy = py - wy*scale;
  syncZoomUI(); draw();
}
function syncZoomUI(){
  zoomSlider.value = Math.min(20, Math.max(0.05, scale));
  scaleOut.textContent = scale>=1 ? scale.toFixed(2)+'×' : (scale.toFixed(3)+'×');
}
zoomSlider.addEventListener('input', ()=>{
  if(!img) return;
  zoomAt(view.width/2, view.height/2, parseFloat(zoomSlider.value));
});

document.getElementById('fit').onclick = ()=> fitMode('fit');
document.getElementById('fill').onclick = ()=> fitMode('fill');
document.getElementById('one').onclick = ()=>{
  scale = frameRect.s / SIZE; // 1 device px = 1 source px inside frame
  centerImage(); syncZoomUI(); draw();
};
function fitMode(mode){
  if(!img) return;
  const fitScale = frameRect.s / Math.max(img.width, img.height);
  const fillScale = frameRect.s / Math.min(img.width, img.height);
  scale = mode==='fit' ? fitScale : fillScale;
  centerImage(); syncZoomUI(); draw();
}
function centerImage(){
  ox = frameRect.x + frameRect.s/2 - img.width*scale/2;
  oy = frameRect.y + frameRect.s/2 - img.height*scale/2;
}

// ---- loading source image ----
function loadImageFromURL(url){
  const im = new Image();
  im.onload = ()=>{
    img = im;
    editing = true;            // a user-loaded image to send — protect from slot view
    enableControls(true);
    fitMode('fill');
    setStatus(`Loaded ${im.width}×${im.height}. Frame it.`, 'ok');
  };
  im.onerror = ()=> setStatus('Could not decode image.', 'err');
  im.src = url;
}
document.getElementById('fileImg').addEventListener('change', e=>{
  const f = e.target.files[0]; if(!f) return;
  loadImageFromURL(URL.createObjectURL(f));
});

function enableControls(on){
  ['zoom','fit','fill','one','savePng','saveDat','sendDev'].forEach(id=>{
    document.getElementById(id).disabled = !on;
  });
}

// ============================================================
//  RGB565 packing  (big-endian: high byte first, M5GFX default)
// ============================================================
function previewToRGB565(){
  const id = pctx.getImageData(0,0,SIZE,SIZE).data;
  const out = new Uint8Array(SIZE*SIZE*2);
  let j=0;
  for(let i=0;i<id.length;i+=4){
    const r=id[i], g=id[i+1], b=id[i+2];
    const v = ((r & 0xF8)<<8) | ((g & 0xFC)<<3) | (b>>3); // 16-bit RGB565
    out[j++] = (v>>8) & 0xFF;  // high byte first  (big-endian)
    out[j++] = v & 0xFF;       // low byte
  }
  return out;
}

function downloadBlob(blob, name){
  const a=document.createElement('a');
  a.href=URL.createObjectURL(blob); a.download=name;
  document.body.appendChild(a); a.click(); a.remove();
}

document.getElementById('saveDat').onclick = ()=>{
  if(!img) return;
  const dat = previewToRGB565();
  downloadBlob(new Blob([dat],{type:'application/octet-stream'}), 'frame.dat');
  setStatus('Exported frame.dat (32768 bytes).', 'ok');
};

// ============================================================
//  SEND -> DEVICE : POST the packed RGB565 to /frame (multipart,
//  binary-safe for the ESP32 core WebServer upload handler).
// ============================================================
async function sendToDevice(){
  if(!img) return;
  const dat = recoveredDat ? recoveredDat : previewToRGB565();
  setStatus('Sending to slot '+(activeSlot+1)+'…');
  try{
    const fd = new FormData();
    fd.append('frame', new Blob([dat],{type:'application/octet-stream'}), 'frame.dat');
    const r = await fetch('/frame?slot='+activeSlot, { method:'POST', body: fd });
    if(r.ok){
      sessionImg[activeSlot] = img;                       // keep the full-res original
      sessionFrame[activeSlot] = { ox, oy, scale };       // ...and its framing
      delete slotCache[activeSlot];   // stale read-back, if any
      editing = false;
      lastFilled[activeSlot] = true;
      buildSlotBar();
      setStatus('Stored on slot '+(activeSlot+1)+' ('+dat.length+' B) — full-res original kept for reframing.', 'ok');
    }
    else setStatus('Device error: HTTP '+r.status, 'err');
  }catch(e){
    setStatus('Send failed — is this page served by the device?', 'err');
  }
}
document.getElementById('sendDev').onclick = sendToDevice;

// ============================================================
//  PNG with embedded private "daTa" chunk
//  We render the 128x128 preview to a PNG, then splice in a
//  custom ancillary chunk carrying the raw RGB565 bytes.
// ============================================================

// CRC32 (PNG polynomial)
const CRC_TABLE = (()=>{
  const t = new Uint32Array(256);
  for(let n=0;n<256;n++){
    let c=n;
    for(let k=0;k<8;k++) c = (c&1)? (0xEDB88320 ^ (c>>>1)) : (c>>>1);
    t[n]=c>>>0;
  }
  return t;
})();
function crc32(bytes){
  let c = 0xFFFFFFFF;
  for(let i=0;i<bytes.length;i++) c = CRC_TABLE[(c ^ bytes[i]) & 0xFF] ^ (c>>>8);
  return (c ^ 0xFFFFFFFF) >>> 0;
}

function makeChunk(type, data){
  // type: 4 ascii chars, data: Uint8Array
  const typeBytes = new Uint8Array([...type].map(ch=>ch.charCodeAt(0)));
  const len = data.length;
  const out = new Uint8Array(12 + len);
  const dv = new DataView(out.buffer);
  dv.setUint32(0, len);                 // length
  out.set(typeBytes, 4);                // type
  out.set(data, 8);                      // data
  const crcInput = new Uint8Array(4 + len);
  crcInput.set(typeBytes,0); crcInput.set(data,4);
  dv.setUint32(8+len, crc32(crcInput)); // crc over type+data
  return out;
}

// Insert chunk just before IEND in an existing PNG ArrayBuffer.
function injectChunk(pngBuf, chunk){
  const png = new Uint8Array(pngBuf);
  // find IEND: walk chunks from offset 8
  let p = 8;
  const dv = new DataView(png.buffer);
  let iendStart = -1;
  while(p < png.length){
    const len = dv.getUint32(p);
    const type = String.fromCharCode(png[p+4],png[p+5],png[p+6],png[p+7]);
    if(type === 'IEND'){ iendStart = p; break; }
    p += 12 + len;
  }
  if(iendStart < 0) throw new Error('No IEND found');
  const out = new Uint8Array(png.length + chunk.length);
  out.set(png.subarray(0, iendStart), 0);
  out.set(chunk, iendStart);
  out.set(png.subarray(iendStart), iendStart + chunk.length);
  return out;
}

document.getElementById('savePng').onclick = ()=>{
  if(!img) return;
  // 1. the visible 128x128 preview becomes the PNG image itself
  pv.toBlob(blob=>{
    blob.arrayBuffer().then(buf=>{
      const dat = previewToRGB565();            // truth = raw bytes
      const chunk = makeChunk('daTa', dat);      // private ancillary chunk
      const merged = injectChunk(buf, chunk);
      downloadBlob(new Blob([merged],{type:'image/png'}), 'frame.png');
      setStatus('Saved frame.png (preview + embedded RGB565).', 'ok');
    });
  }, 'image/png');
};

// ---- loading a previously-saved PNG: extract daTa chunk if present ----
document.getElementById('filePng').addEventListener('change', e=>{
  const f = e.target.files[0]; if(!f) return;
  f.arrayBuffer().then(buf=>{
    const png = new Uint8Array(buf);
    const dv = new DataView(buf);
    let p = 8, found = null;
    try{
      while(p < png.length){
        const len = dv.getUint32(p);
        const type = String.fromCharCode(png[p+4],png[p+5],png[p+6],png[p+7]);
        if(type === 'daTa'){ found = png.subarray(p+8, p+8+len); break; }
        if(type === 'IEND') break;
        p += 12 + len;
      }
    }catch(err){ /* malformed; fall through */ }

    // Always also load the visible image so the operator sees it & can re-frame.
    loadImageFromURL(URL.createObjectURL(f));

    if(found && found.length === SIZE*SIZE*2){
      // stash recovered data so EXPORT .DAT yields the exact original bytes
      recoveredDat = new Uint8Array(found);
      setStatus('PNG loaded — embedded RGB565 recovered (32768 B).', 'ok');
    }else{
      recoveredDat = null;
      setStatus('PNG loaded (no embedded data — re-frame & save).', 'ok');
    }
  });
});

// If we recovered exact bytes, EXPORT .DAT should prefer them over re-sampling.
let recoveredDat = null;
// Override export to use recovered bytes when the frame hasn't been touched:
document.getElementById('saveDat').onclick = ()=>{
  if(!img) return;
  const dat = recoveredDat ? recoveredDat : previewToRGB565();
  downloadBlob(new Blob([dat],{type:'application/octet-stream'}), 'frame.dat');
  setStatus('Exported frame.dat (32768 bytes)'+(recoveredDat?' — from embedded data.':'.'), 'ok');
};
// any reframing invalidates recovered bytes
['mousemove','wheel'].forEach(ev=>view.addEventListener(ev, ()=>{ if(dragging||ev==='wheel'){ recoveredDat=null; editing=true; } }));

// ============================================================
//  Slots + button gestures
// ============================================================
const NSLOTS = 4, NGEST = 3, BYTES = SIZE*SIZE*2;
let activeSlot = 0;
let lastFilled = [false,false,false,false];
let gestureLines = new Array(NSLOTS*NGEST).fill(''); // slot*NGEST + gesture
const sessionImg = {};       // slot -> HTMLImageElement (full-res session original)
const sessionFrame = {};     // slot -> {ox,oy,scale} framing of that original
const slotCache = {};        // slot -> HTMLImageElement (low-res device read-back)
let editing = false;         // true once the user loads/reframes their own image

function buildSlotBar(){
  const bar = document.getElementById('slotbar');
  bar.innerHTML = '';
  for(let i=0;i<NSLOTS;i++){
    const b = document.createElement('button');
    b.className = 'slotbtn' + (i===activeSlot?' active':'') + (lastFilled[i]?' filled':'');
    b.textContent = (i+1);
    b.title = lastFilled[i] ? 'slot '+(i+1)+' (filled)' : 'slot '+(i+1)+' (empty)';
    b.onclick = ()=> selectSlot(i);
    bar.appendChild(b);
  }
}
function fillGestureInputs(){
  document.getElementById('gSlot').textContent = (activeSlot+1);
  document.querySelectorAll('.gurl').forEach(inp=>{
    inp.value = gestureLines[activeSlot*NGEST + (+inp.dataset.g)] || '';
  });
}
function selectSlot(n){
  activeSlot = n;
  buildSlotBar();
  fillGestureInputs();
  if(!editing) viewSlot(n);
}

// Decode big-endian RGB565 bytes into an offscreen canvas (SIZE x SIZE).
function rgb565ToCanvas(bytes){
  const c = document.createElement('canvas'); c.width = SIZE; c.height = SIZE;
  const cx = c.getContext('2d');
  const id = cx.createImageData(SIZE, SIZE);
  let j=0;
  for(let i=0;i<bytes.length;i+=2){
    const v = (bytes[i]<<8) | bytes[i+1];
    const r5=(v>>11)&0x1F, g6=(v>>5)&0x3F, b5=v&0x1F;
    id.data[j++]=(r5*255/31)|0; id.data[j++]=(g6*255/63)|0; id.data[j++]=(b5*255/31)|0; id.data[j++]=255;
  }
  cx.putImageData(id,0,0);
  return c;
}
// Fetch a slot's bytes once, buffer as a PNG Image, cache it.
async function fetchSlotImage(slot){
  if(slotCache[slot]) return slotCache[slot];
  const buf = await (await fetch('/frame?slot='+slot)).arrayBuffer();
  const bytes = new Uint8Array(buf);
  if(bytes.length !== BYTES) throw new Error('bad size');
  const png = rgb565ToCanvas(bytes).toDataURL('image/png');
  const im = await new Promise((res,rej)=>{ const x=new Image(); x.onload=()=>res(x); x.onerror=rej; x.src=png; });
  slotCache[slot] = im;
  return im;
}
// Show a slot's stored image in the large stage + preview (from cache).
async function viewSlot(slot){
  if(sessionImg[slot]){               // prefer the full-res original from this session
    img = sessionImg[slot]; recoveredDat = null; editing = false;
    enableControls(true);
    const fr = sessionFrame[slot];
    if(fr){ scale = fr.scale; ox = fr.ox; oy = fr.oy; syncZoomUI(); draw(); }
    else fitMode('fill');
    setStatus('Slot '+(slot+1)+' (session original, full-res — reframe & re-send anytime).', 'ok');
    return;
  }
  if(!lastFilled[slot]){ img=null; enableControls(false); pctx.clearRect(0,0,SIZE,SIZE); draw(); setStatus('Slot '+(slot+1)+' is empty — load an image and SEND.'); return; }
  try{
    const im = await fetchSlotImage(slot);
    if(editing || sessionImg[slot]) return;
    img = im; recoveredDat = null;
    enableControls(true);
    fitMode('fill');                  // low-res read-back, covers the area
    setStatus('Slot '+(slot+1)+' (from device, low-res). Load an image to replace.', 'ok');
  }catch(e){ /* offline or empty */ }
}

async function loadState(){
  try{
    const s = await (await fetch('/state')).json();
    if(Array.isArray(s.filled)) lastFilled = s.filled;
    if(typeof s.slot === 'number') activeSlot = s.slot;
    if(s.name) document.getElementById('devName').textContent = s.name;
    buildSlotBar();
    fillGestureInputs();
    if(!editing) viewSlot(activeSlot);
  }catch(_){ buildSlotBar(); }
}
async function loadGestures(){
  try{
    const txt = await (await fetch('/buttons')).text();
    const lines = txt.split('\n');
    for(let i=0;i<NSLOTS*NGEST;i++) gestureLines[i] = (lines[i]||'').replace(/\r$/,'');
  }catch(_){}
  fillGestureInputs();
}
async function saveGestures(){
  document.querySelectorAll('.gurl').forEach(inp=>{
    gestureLines[activeSlot*NGEST + (+inp.dataset.g)] = inp.value.trim();
  });
  try{
    const r = await fetch('/buttons', { method:'POST',
      headers:{'Content-Type':'text/plain'}, body: gestureLines.join('\n') });
    setStatus(r.ok ? 'Gestures saved.' : 'Save failed: HTTP '+r.status, r.ok?'ok':'err');
  }catch(e){ setStatus('Save failed — page must be served by the device.', 'err'); }
}
async function showOnDevice(){
  try{
    const r = await fetch('/show?slot='+activeSlot);
    setStatus(r.ok ? 'Showing slot '+(activeSlot+1)+' on device.' : 'Slot '+(activeSlot+1)+' is empty — send an image first.', r.ok?'ok':'err');
  }catch(e){ setStatus('Show failed — page must be served by the device.', 'err'); }
}
document.getElementById('saveBtns').onclick = saveGestures;
document.getElementById('showDev').onclick = showOnDevice;

// ---- fleet peers ----
let lastField = null;
document.querySelectorAll('.gurl').forEach(i=> i.addEventListener('focus', ()=> lastField=i));
async function loadPeers(){
  let list; try{ list = await (await fetch('/peers')).json(); }catch(_){ return; }
  const box = document.getElementById('peers');
  if(!list.length){ box.innerHTML = '<span class="hint2">no other devices seen yet</span>'; return; }
  box.innerHTML = '';
  for(const p of list){
    const row = document.createElement('div');
    row.className = 'peerrow';
    row.innerHTML = '<span class="pn"></span><span class="pip"></span>';
    row.querySelector('.pn').textContent = p.name;
    row.querySelector('.pip').textContent = p.ip + ' · ' + p.age + 's';
    row.onclick = ()=>{
      if(lastField){ lastField.value = p.name+':1'; lastField.focus(); setStatus('Put '+p.name+':1 — edit the slot # and SAVE GESTURES.', 'ok'); }
      else setStatus('Focus a gesture field first, then click a peer.', 'err');
    };
    box.appendChild(row);
  }
}

// ---- boot ----
resizeView();
buildSlotBar();
loadState();
loadGestures();
loadPeers();
setInterval(loadPeers, 5000);
setStatus('Load an image to begin.');
</script>
</body>
</html>
)=====";
