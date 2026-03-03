#pragma once

// ============================================================================
// Unified Dashboard HTML (PROGMEM)
// Single-page app with tabs for System, Detector, Foxhunter, Flock-You, Sky Spy
//
// Note: This firmware runs on an ESP32-S3 serving its own WiFi AP (no internet).
// All data comes from the device's own trusted JSON APIs. The innerHTML usage
// is safe here because the content is generated from device-local data only,
// not from arbitrary user input or external sources. This is an embedded device
// dashboard, not a public web application.
// ============================================================================

static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>OUI SPY</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
html,body{height:100%;overflow:hidden}
body{font-family:'Courier New',monospace;background:#000;color:#0f0;display:flex;flex-direction:column}
.hd{background:#000;padding:8px 12px;border-bottom:2px solid #0f0;flex-shrink:0}
.hd h1{font-size:20px;color:#0f0;letter-spacing:3px;text-align:center}
.hd .sub{font-size:10px;color:#0a0;text-align:center;margin-top:2px}
.tb{display:flex;border-bottom:1px solid #0a0;flex-shrink:0;overflow-x:auto}
.tb button{flex:1;padding:8px 4px;text-align:center;cursor:pointer;color:#0a0;border:none;background:none;font-family:inherit;font-size:11px;font-weight:bold;letter-spacing:1px;white-space:nowrap;min-width:0}
.tb button.a{color:#0f0;border-bottom:2px solid #0f0;background:rgba(0,255,0,.05)}
.cn{flex:1;overflow-y:auto;padding:8px}
.pn{display:none}.pn.a{display:block}
.card{background:rgba(0,40,0,.4);border:1px solid #0a0;border-radius:5px;padding:8px;margin-bottom:6px}
.card h3{color:#0f0;font-size:13px;margin-bottom:4px}
.card .val{color:#fff;font-size:12px}
.row{display:flex;gap:6px;margin-bottom:6px}
.row .col{flex:1}
.stat{text-align:center;padding:6px;border:1px solid #0a0;border-radius:5px}
.stat .n{font-size:20px;font-weight:bold;color:#0f0}
.stat .l{font-size:9px;color:#0a0;margin-top:2px}
.tog{display:flex;align-items:center;justify-content:space-between;padding:6px 8px;border:1px solid #0a0;border-radius:5px;margin-bottom:4px;cursor:pointer}
.tog .nm{font-size:12px;color:#0f0}
.tog .sw{width:36px;height:18px;border-radius:9px;position:relative;transition:.3s}
.tog .sw.on{background:#0f0}.tog .sw.off{background:#333}
.tog .sw::after{content:'';position:absolute;width:14px;height:14px;border-radius:50%;background:#fff;top:2px;transition:.3s}
.tog .sw.on::after{left:20px}.tog .sw.off::after{left:2px}
.btn{display:block;width:100%;padding:8px;margin-bottom:6px;background:#0a0;color:#000;border:none;border-radius:5px;cursor:pointer;font-family:inherit;font-size:12px;font-weight:bold}
.btn:active{background:#0f0}
.btn.sm{display:inline-block;width:auto;padding:4px 12px;font-size:11px}
input[type=text]{width:100%;padding:6px;background:#001a00;color:#0f0;border:1px solid #0a0;border-radius:4px;font-family:inherit;font-size:12px;margin-bottom:4px}
input:focus{outline:none;border-color:#0f0}
textarea{width:100%;padding:6px;background:#001a00;color:#0f0;border:1px solid #0a0;border-radius:4px;font-family:inherit;font-size:11px;min-height:60px;resize:vertical}
.det{background:rgba(0,40,0,.4);border:1px solid #0a0;border-radius:5px;padding:6px;margin-bottom:4px}
.det .mac{color:#0f0;font-weight:bold;font-size:12px}
.det .inf{display:flex;flex-wrap:wrap;gap:4px;margin-top:3px;font-size:10px}
.det .inf span{background:rgba(0,60,0,.5);padding:2px 5px;border-radius:3px}
.empty{text-align:center;color:#060;padding:20px;font-size:12px}
label{font-size:11px;color:#0a0;display:block;margin-bottom:2px}
.sep{border:none;border-top:1px solid #0a0;margin:8px 0}
</style></head><body>
<div class="hd"><h1>OUI SPY</h1><div class="sub" id="uptime">UNIFIED FIRMWARE</div></div>
<div class="tb">
<button class="a" onclick="tab(0,this)">SYSTEM</button>
<button onclick="tab(1,this)">DETECT</button>
<button onclick="tab(2,this)">FOX</button>
<button onclick="tab(3,this)">FLOCK</button>
<button onclick="tab(4,this)">SKY</button>
</div>
<div class="cn">

<!-- SYSTEM TAB -->
<div class="pn a" id="p0">
<div class="row">
<div class="stat col"><div class="n" id="sHeap">-</div><div class="l">HEAP KB</div></div>
<div class="stat col"><div class="n" id="sPsram">-</div><div class="l">PSRAM KB</div></div>
<div class="stat col"><div class="n" id="sUp">-</div><div class="l">UPTIME</div></div>
</div>
<div class="card"><h3>MODULES</h3><div id="modList"></div></div>
<div class="card"><h3>BUZZER</h3>
<div class="tog" onclick="toggleBuzzer()"><span class="nm">Buzzer Audio</span><div class="sw" id="bzSw"></div></div>
</div>
<div class="card"><h3>GPS</h3><div id="gpsInfo" class="val">Loading...</div></div>
<div class="card"><h3>AP SETTINGS</h3>
<label>SSID</label><input type="text" id="apSSID" maxlength="32">
<label>Password</label><input type="text" id="apPass" maxlength="63">
<button class="btn" onclick="saveAP()">SAVE &amp; REBOOT</button>
</div>
<button class="btn" style="background:#800;color:#fff;margin-top:8px" onclick="if(confirm('Restart device?'))fetch('/api/reset',{method:'POST'})">RESTART DEVICE</button>
</div>

<!-- DETECTOR TAB -->
<div class="pn" id="p1">
<div class="card"><h3>FILTERS</h3>
<label>OUI Prefixes (one per line)</label><textarea id="detOuis" placeholder="AA:BB:CC"></textarea>
<label>Full MACs (one per line)</label><textarea id="detMacs" placeholder="AA:BB:CC:DD:EE:FF"></textarea>
<button class="btn" onclick="saveDetFilters()">SAVE FILTERS</button>
</div>
<div class="card"><h3>DETECTED DEVICES</h3><div id="detList"><div class="empty">No detections yet</div></div></div>
</div>

<!-- FOXHUNTER TAB -->
<div class="pn" id="p2">
<div class="row">
<div class="stat col"><div class="n" id="foxRSSI">-</div><div class="l">RSSI dBm</div></div>
<div class="stat col"><div class="n" id="foxDet">-</div><div class="l">STATUS</div></div>
</div>
<div class="card"><h3>TARGET MAC</h3>
<input type="text" id="foxMAC" placeholder="AA:BB:CC:DD:EE:FF" maxlength="17">
<div class="row"><button class="btn sm" onclick="setFoxTarget()">SET TARGET</button>
<button class="btn sm" style="background:#800;color:#fff" onclick="clearFoxTarget()">CLEAR</button></div>
</div>
<div class="card"><h3>RSSI METER</h3>
<div style="height:20px;background:#001a00;border:1px solid #0a0;border-radius:3px;overflow:hidden">
<div id="foxBar" style="height:100%;background:#0f0;width:0%;transition:.3s"></div>
</div>
</div>
</div>

<!-- FLOCK-YOU TAB -->
<div class="pn" id="p3">
<div class="row">
<div class="stat col"><div class="n" id="fyTotal">0</div><div class="l">DETECTED</div></div>
<div class="stat col"><div class="n" id="fyRaven">0</div><div class="l">RAVEN</div></div>
<div class="stat col"><div class="n" id="fyGPS" style="font-size:12px">-</div><div class="l">GPS</div></div>
</div>
<div id="fyList"><div class="empty">Scanning...</div></div>
<hr class="sep">
<button class="btn" onclick="location.href='/api/flockyou/export/json'">EXPORT JSON</button>
<button class="btn" onclick="location.href='/api/flockyou/export/csv'">EXPORT CSV</button>
<button class="btn" style="background:#060" onclick="location.href='/api/flockyou/export/kml'">EXPORT KML</button>
<button class="btn" style="background:#800;color:#fff" onclick="if(confirm('Clear all?'))fetch('/api/flockyou/clear').then(function(){fyRefresh()})">CLEAR ALL</button>
</div>

<!-- SKY SPY TAB -->
<div class="pn" id="p4">
<div class="row">
<div class="stat col"><div class="n" id="ssCount">0</div><div class="l">DRONES</div></div>
<div class="stat col"><div class="n" id="ssRange">-</div><div class="l">STATUS</div></div>
</div>
<div id="ssList"><div class="empty">Scanning for drone Remote ID...</div></div>
</div>

</div>
<script>
function tab(i,el){document.querySelectorAll('.tb button').forEach(function(b){b.classList.remove('a')});document.querySelectorAll('.pn').forEach(function(p){p.classList.remove('a')});el.classList.add('a');document.getElementById('p'+i).classList.add('a');}

// Helper to safely build text content
function ce(tag,cls,txt){var e=document.createElement(tag);if(cls)e.className=cls;if(txt)e.textContent=txt;return e;}

// SYSTEM
function sysRefresh(){
fetch('/api/status').then(function(r){return r.json()}).then(function(d){
document.getElementById('sHeap').textContent=Math.round(d.heap/1024);
document.getElementById('sPsram').textContent=Math.round(d.psram/1024);
var m=Math.floor(d.uptime/60),h=Math.floor(m/60);
document.getElementById('sUp').textContent=h>0?h+'h'+('0'+(m%60)).slice(-2)+'m':m+'m';
document.getElementById('uptime').textContent='UP '+d.uptime+'s | HEAP '+Math.round(d.heap/1024)+'KB';
var bz=document.getElementById('bzSw');bz.className='sw '+(d.buzzer?'on':'off');
}).catch(function(){});
fetch('/api/modules').then(function(r){return r.json()}).then(function(mods){
var el=document.getElementById('modList');el.textContent='';
mods.forEach(function(m){
var tog=document.createElement('div');tog.className='tog';
tog.setAttribute('onclick','toggleMod("'+m.name+'",'+(!m.enabled)+')');
var nm=ce('span','nm',m.name.toUpperCase());
var sw=ce('div','sw '+(m.enabled?'on':'off'));
tog.appendChild(nm);tog.appendChild(sw);el.appendChild(tog);
});
}).catch(function(){});
fetch('/api/gps').then(function(r){return r.json()}).then(function(g){
var s='';if(g.hw_fix)s='HW GPS: '+g.sats+' sats, '+g.lat.toFixed(5)+','+g.lon.toFixed(5);
else if(g.fresh)s='Phone GPS: '+g.lat.toFixed(5)+','+g.lon.toFixed(5);
else if(g.hw_detected)s='HW GPS: '+g.sats+' sats (no fix)';
else s='No GPS';
document.getElementById('gpsInfo').textContent=s;
}).catch(function(){});
fetch('/api/ap').then(function(r){return r.json()}).then(function(d){document.getElementById('apSSID').value=d.ssid;}).catch(function(){});}

function toggleMod(name,en){fetch('/api/modules/toggle',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'name='+name+'&enabled='+(en?'true':'false')}).then(function(){sysRefresh()});}
function toggleBuzzer(){var bz=document.getElementById('bzSw');var on=bz.classList.contains('off');fetch('/api/buzzer',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'on='+(on?'true':'false')}).then(function(){sysRefresh()});}
function saveAP(){var s=document.getElementById('apSSID').value,p=document.getElementById('apPass').value;if(!s){alert('SSID required');return;}fetch('/api/ap',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p)}).then(function(r){if(r.ok)alert('Saved! Rebooting...');});}

// DETECTOR
function detRefresh(){
fetch('/api/detector/devices').then(function(r){return r.json()}).then(function(d){
var el=document.getElementById('detList');
el.textContent='';
if(!d.devices||!d.devices.length){el.appendChild(ce('div','empty','No detections'));return;}
d.devices.forEach(function(v){
var div=document.createElement('div');div.className='det';
var mac=ce('div','mac',v.mac+(v.alias?' ('+v.alias+')':''));
var inf=document.createElement('div');inf.className='inf';
inf.appendChild(ce('span','',v.rssi+' dBm'));
inf.appendChild(ce('span','',v.filter));
div.appendChild(mac);div.appendChild(inf);el.appendChild(div);
});
}).catch(function(){});
fetch('/api/detector/filters').then(function(r){return r.json()}).then(function(f){
var ouis=[],macs=[];f.forEach(function(x){if(x.full)macs.push(x.id);else ouis.push(x.id);});
document.getElementById('detOuis').value=ouis.join('\n');
document.getElementById('detMacs').value=macs.join('\n');
}).catch(function(){});}
function saveDetFilters(){var fd=new URLSearchParams();fd.append('ouis',document.getElementById('detOuis').value);fd.append('macs',document.getElementById('detMacs').value);fetch('/api/detector/filters',{method:'POST',body:fd}).then(function(r){return r.json()}).then(function(d){alert('Saved '+d.saved+' filters')}).catch(function(){alert('Error')});}

// FOXHUNTER
function foxRefresh(){
fetch('/api/foxhunter/status').then(function(r){return r.json()}).then(function(d){
document.getElementById('foxRSSI').textContent=d.detected?d.rssi:'-';
document.getElementById('foxDet').textContent=d.detected?'TRACKING':'SEARCHING';
document.getElementById('foxDet').style.color=d.detected?'#0f0':'#0a0';
if(d.target)document.getElementById('foxMAC').value=d.target;
var pct=d.detected?Math.min(100,Math.max(0,((d.rssi+100)/70)*100)):0;
document.getElementById('foxBar').style.width=pct+'%';
}).catch(function(){});}
function setFoxTarget(){var m=document.getElementById('foxMAC').value.trim();if(!m){alert('Enter MAC');return;}fetch('/api/foxhunter/target',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'mac='+encodeURIComponent(m)}).then(function(){foxRefresh()});}
function clearFoxTarget(){fetch('/api/foxhunter/clear',{method:'POST'}).then(function(){document.getElementById('foxMAC').value='';foxRefresh();});}

// FLOCK-YOU
function fyRefresh(){
fetch('/api/flockyou/detections').then(function(r){return r.json()}).then(function(d){
document.getElementById('fyTotal').textContent=d.length;
document.getElementById('fyRaven').textContent=d.filter(function(x){return x.raven}).length;
var el=document.getElementById('fyList');
el.textContent='';
if(!d.length){el.appendChild(ce('div','empty','Scanning...'));return;}
d.sort(function(a,b){return b.last-a.last});
d.forEach(function(x){
var div=document.createElement('div');div.className='det';
var mac=ce('div','mac',x.mac+(x.name?' '+x.name:''));
var inf=document.createElement('div');inf.className='inf';
inf.appendChild(ce('span','',x.rssi+''));
inf.appendChild(ce('span','',x.method));
inf.appendChild(ce('span','','x'+x.count));
if(x.raven){var rv=ce('span','','RAVEN '+(x.fw||''));rv.style.color='#f44';inf.appendChild(rv);}
if(x.gps){var gp=ce('span','','GPS');gp.style.color='#0f0';inf.appendChild(gp);}
div.appendChild(mac);div.appendChild(inf);el.appendChild(div);
});
}).catch(function(){});
fetch('/api/flockyou/stats').then(function(r){return r.json()}).then(function(s){
var g=document.getElementById('fyGPS');
if(s.gps_src==='hw'){g.textContent=s.gps_sats+'sat';g.style.color='#0f0';}
else if(s.gps_src==='phone'){g.textContent='PHONE';g.style.color='#0f0';}
else{g.textContent='OFF';g.style.color='#600';}
}).catch(function(){});}

// SKY SPY
function ssRefresh(){
fetch('/api/skyspy/drones').then(function(r){return r.json()}).then(function(d){
document.getElementById('ssCount').textContent=d.length;
var el=document.getElementById('ssList');
el.textContent='';
if(!d.length){el.appendChild(ce('div','empty','Scanning for drones...'));return;}
d.forEach(function(x){
var div=document.createElement('div');div.className='det';
var mac=ce('div','mac',x.mac);
var inf=document.createElement('div');inf.className='inf';
inf.appendChild(ce('span','','RSSI:'+x.rssi));
if(x.uav_id)inf.appendChild(ce('span','','ID:'+x.uav_id));
if(x.drone_lat)inf.appendChild(ce('span','',x.drone_lat.toFixed(4)+','+x.drone_long.toFixed(4)));
if(x.altitude)inf.appendChild(ce('span','','Alt:'+x.altitude+'m'));
if(x.speed)inf.appendChild(ce('span','',x.speed+'km/h'));
div.appendChild(mac);div.appendChild(inf);el.appendChild(div);
});
}).catch(function(){});
fetch('/api/skyspy/status').then(function(r){return r.json()}).then(function(s){
document.getElementById('ssRange').textContent=s.in_range?'IN RANGE':'CLEAR';
document.getElementById('ssRange').style.color=s.in_range?'#0f0':'#0a0';
}).catch(function(){});}

// Polling
sysRefresh();detRefresh();foxRefresh();fyRefresh();ssRefresh();
setInterval(sysRefresh,5000);setInterval(detRefresh,3000);setInterval(foxRefresh,500);setInterval(fyRefresh,2500);setInterval(ssRefresh,2000);
</script></body></html>
)rawliteral";
