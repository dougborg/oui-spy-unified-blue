#pragma once

// Captive portal welcome page — served on HTTP :80
// Lightweight static HTML (no Preact SPA) for the captive portal mini-browser
static const char WELCOME_HTML[] = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>OUI SPY</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Courier New',monospace;background:#0a0a0a;color:#00ff41;
min-height:100vh;display:flex;align-items:center;justify-content:center}
.c{text-align:center;padding:24px;max-width:400px}
h1{font-size:28px;letter-spacing:4px;margin-bottom:8px}
.sub{font-size:11px;color:#00aa00;margin-bottom:32px}
a.btn{display:block;padding:14px;margin:10px 0;border:2px solid #00ff41;
color:#000;background:#00aa00;text-decoration:none;font-weight:bold;
font-size:14px;letter-spacing:2px;font-family:inherit}
a.btn:active{background:#00ff41}
a.sec{border-color:#1a3a1a;background:transparent;color:#00ff41;font-size:12px}
.note{font-size:10px;color:#006600;margin-top:24px;line-height:1.6}
</style>
</head>
<body>
<div class="c">
<h1>OUI SPY</h1>
<div class="sub">UNIFIED SURVEILLANCE DETECTION</div>
<a class="btn" href="https://ouispy.local/" target="_blank">OPEN DASHBOARD</a>
<a class="btn sec" href="/ca.cer">DOWNLOAD CA CERTIFICATE</a>
<div class="note">
For HTTPS without warnings:<br>
1. Tap "Download CA Certificate"<br>
2. iOS: Settings &gt; Downloaded Profile &gt; Install<br>
3. Settings &gt; General &gt; About &gt; Certificate Trust Settings &gt; Enable<br>
4. Then open the dashboard link above
</div>
</div>
</body>
</html>)rawliteral";
