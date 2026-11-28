#pragma once

#include <Arduino.h>

// 脉冲热式三通道液位传感器监控与配置中心 — SPA 暗色前端 HTML（支持每通道独立标定与阈值）
static const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>热式液位传感器监控中心</title>
<style>
:root{--bg:#0b0f19;--card:rgba(22,30,49,.85);--border:rgba(255,255,255,.08);--text:#f1f5f9;--muted:#64748b;--blue:#38bdf8;--cyan:#06b6d4;--green:#10b981;--red:#f43f5e;--yellow:#f59e0b;--nav:rgba(17,24,39,.97)}
*{box-sizing:border-box;margin:0;padding:0;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif}
body{background:var(--bg);color:var(--text);min-height:100vh;display:flex;flex-direction:column;align-items:center}
header{width:100%;background:var(--nav);border-bottom:1px solid var(--border);padding:.6rem 1rem;position:sticky;top:0;z-index:100;backdrop-filter:blur(8px)}
.header-main{max-width:860px;margin:0 auto;display:flex;align-items:center;justify-content:space-between;width:100%;gap:.5rem;flex-wrap:wrap}
.logo{font-size:1rem;font-weight:700;letter-spacing:.4px;background:linear-gradient(135deg,var(--blue),var(--cyan));-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.status-dots{display:flex;gap:.65rem;align-items:center;font-size:.78rem;color:var(--muted);background:rgba(0,0,0,.3);padding:.25rem .6rem;border-radius:20px;border:1px solid var(--border)}
.status-indicator{display:flex;align-items:center;gap:.3rem}
.dot{width:8px;height:8px;border-radius:50%;display:inline-block;background:#64748B;transition:all .3s ease}
.dot.on{background:var(--green);box-shadow:0 0 8px var(--green)}
.dot.off{background:var(--red);box-shadow:0 0 4px var(--red)}
.nav-tabs{display:flex;width:100%;background-color:var(--card);border:1px solid var(--border);border-radius:10px;padding:.25rem;margin-bottom:.75rem;gap:.25rem}
.tab-item{flex:1;text-align:center;padding:.5rem .4rem;font-size:.84rem;font-weight:600;color:var(--muted);border-radius:8px;cursor:pointer;transition:all .2s ease;user-select:none}
.tab-item:hover{color:var(--text)}
.tab-item.active{background:linear-gradient(135deg,rgba(56,189,248,.18),rgba(6,182,212,.18));color:var(--blue);border:1px solid rgba(56,189,248,.35)}
.footer{text-align:center;font-size:.74rem;color:var(--muted);margin-top:.85rem;border-top:1px solid var(--border);padding-top:.65rem;line-height:1.6}
main{width:100%;max-width:860px;padding:.85rem .75rem 1.5rem}
.tab-panel{display:none}.tab-panel.active{display:block}
.card{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:1rem 1.15rem;margin-bottom:.85rem;backdrop-filter:blur(12px)}
.card-title{font-size:.76rem;text-transform:uppercase;letter-spacing:.08em;color:var(--muted);margin-bottom:.65rem}
.mon-card{padding:.65rem .75rem}
.mon-bar{display:flex;justify-content:space-between;align-items:center;font-size:.78rem;color:var(--muted);margin-bottom:.5rem;padding:0 .2rem;gap:.5rem;flex-wrap:wrap}
.mon-bar strong{color:var(--blue);font-weight:600}
.tbl-wrap{width:100%;overflow-x:auto}
.cmp-tbl{width:100%;border-collapse:collapse;table-layout:fixed;font-variant-numeric:tabular-nums}
.cmp-tbl th,.cmp-tbl td{padding:.45rem .35rem;text-align:center;border-bottom:1px solid rgba(255,255,255,.05);font-size:.86rem}
.cmp-tbl tr:last-child td{border-bottom:none}
.cmp-tbl th{font-weight:600;color:var(--text);border-bottom:1px solid var(--border);vertical-align:bottom;border-top:2px solid transparent;transition:border-color .2s,background .2s}
.cmp-tbl th.ch-active{border-top-color:var(--blue);background:rgba(56,189,248,.07)}
.cmp-tbl th.ch-heat,.cmp-tbl td.ch-heat{background:rgba(245,158,11,.08)}
.cmp-tbl .lbl{width:23%;text-align:left;color:var(--muted);font-size:.78rem;font-weight:500;padding-left:.4rem;white-space:nowrap}
.ch-id{display:block;font-size:.88rem;font-weight:700}
.ch-sub{display:block;font-size:.7rem;color:var(--muted);font-weight:400;margin-top:1px}
.dt-cell{font-size:1.12rem!important;font-weight:700;color:var(--blue)}
.dt-cell.liquid{color:var(--cyan)}
.dt-cell.fault{color:var(--red)}
.sub-hint{display:block;font-size:.68rem;color:var(--muted);font-weight:400}
.badge{font-size:.72rem;padding:2px 8px;border-radius:20px;font-weight:600;display:inline-block;white-space:nowrap}
.badge.triggered{background:rgba(6,182,212,.18);color:var(--cyan)}
.badge.idle{background:rgba(100,116,139,.15);color:var(--muted)}
.badge.warn{background:rgba(245,158,11,.18);color:var(--yellow)}
.badge.fault{background:rgba(244,63,94,.18);color:var(--red)}
.ht-on{color:var(--yellow);font-weight:600}
@media(max-width:480px){
  .cmp-tbl th,.cmp-tbl td{padding:.4rem .2rem;font-size:.8rem}
  .cmp-tbl .lbl{width:25%;font-size:.74rem;padding-left:.25rem}
  .dt-cell{font-size:1rem!important}
}
.btn-row{display:flex;gap:.5rem;margin-top:.8rem;flex-wrap:wrap}
.btn{padding:.45rem .95rem;border-radius:8px;border:1px solid var(--border);cursor:pointer;font-size:.8rem;font-weight:600;transition:all .15s;background:rgba(255,255,255,.04);color:var(--text)}
.btn:hover{background:rgba(255,255,255,.1)}
.btn.primary{background:rgba(56,189,248,.15);color:var(--blue);border-color:rgba(56,189,248,.3)}
.btn.primary:hover{background:rgba(56,189,248,.25)}
.btn.danger{background:rgba(244,63,94,.1);color:var(--red);border-color:rgba(244,63,94,.25)}
.btn.danger:hover{background:rgba(244,63,94,.2)}
.fg{margin-bottom:.85rem}
.fg label{display:block;font-size:.8rem;color:var(--muted);margin-bottom:.35rem}
.fg input,.fg select{width:100%;padding:.5rem .75rem;background:rgba(255,255,255,.05);border:1px solid var(--border);border-radius:8px;color:var(--text);font-size:.88rem;outline:none;transition:border-color .2s}
.fg select option{background:#111827;color:var(--text)}
.fg input:focus,.fg select:focus{border-color:var(--blue)}
.form-row{display:grid;grid-template-columns:1fr 1fr;gap:.85rem}
#toast{position:fixed;bottom:1.5rem;right:1.5rem;padding:.7rem 1.2rem;background:rgba(22,30,49,.97);border:1px solid var(--border);border-radius:10px;font-size:.85rem;color:var(--text);transform:translateY(80px);opacity:0;transition:all .3s;pointer-events:none;z-index:999;max-width:280px}
#toast.show{transform:translateY(0);opacity:1}
#toast.ok{border-color:rgba(16,185,129,.4);color:var(--green)}
#toast.err{border-color:rgba(244,63,94,.4);color:var(--red)}
.wifi-list{max-height:200px;overflow-y:auto}
.wi{display:flex;justify-content:space-between;padding:.5rem .4rem;border-bottom:1px solid var(--border);cursor:pointer;border-radius:6px;transition:background .15s}
.wi:hover{background:rgba(255,255,255,.05)}
.wi span{font-size:.85rem}
.wi .rs{color:var(--muted);font-size:.78rem}
</style>
</head>
<body>
<header>
  <div class="header-main">
    <div class="logo">🌡️ 脉冲热式液位传感器节点</div>
    <div class="status-dots">
      <span class="status-indicator" id="bus-v5">5V: --V</span>
      <span class="status-indicator"><span class="dot" id="dot-wifi"></span>WiFi</span>
      <span class="status-indicator"><span class="dot" id="dot-mqtt"></span>MQTT</span>
    </div>
  </div>
</header>
<main>
  <nav class="nav-tabs">
    <div class="tab-item active" onclick="showTab('monitor',this)">实时监控</div>
    <div class="tab-item" onclick="showTab('calibrate',this)">通道标定与阈值</div>
    <div class="tab-item" onclick="showTab('network',this)">网络与系统</div>
  </nav>
  <div id="tab-monitor" class="tab-panel active">
    <div class="card mon-card">
      <div class="mon-bar">
        <span>当前轮巡：<strong id="mon-active">--</strong></span>
        <span id="mon-cal">等待数据...</span>
      </div>
      <div class="tbl-wrap" id="sg"></div>
    </div>
  </div>
  <div id="tab-calibrate" class="tab-panel">
    <div class="card">
      <div class="card-title">冷却衰减比阈值与可选空管调参</div>
      <p style="font-size:.82rem;color:var(--muted);margin-bottom:.75rem">本固件采用免标定冷却衰减比算法（frac_10_30 = 前10s降温 / 30s总降温），空气约 36~42%，水中约 58~63%，上电即用。如需针对新封装自动微调空管基准，可在排空后点击下方自动调参，或直接修改每通道阈值。</p>
      <div id="cal-banner" style="font-size:.85rem;margin-bottom:.75rem;color:var(--cyan)"></div>
      <div class="btn-row">
        <button class="btn primary" onclick="doCal('start',-1)">全部空管自动调参</button>
        <button class="btn danger" onclick="doCal('reset',-1)">恢复默认阈值</button>
      </div>
    </div>
    <div id="ch-cfg-list"></div>
  </div>
  <div id="tab-network" class="tab-panel">
    <div class="card">
      <div class="card-title">WiFi 与 MQTT 配置</div>
      <div class="fg"><label>STA WiFi SSID</label><input type="text" id="net-ssid" placeholder="WiFi 名称"></div>
      <div class="fg"><label>STA WiFi 密码</label><input type="password" id="net-pass" placeholder="留空表示保持不变"></div>
      <div class="fg"><label>设备名称 (MQTT)</label><input type="text" id="net-name" placeholder="例如 home"></div>
      <div class="form-row">
        <div class="fg"><label>MQTT Broker</label><input type="text" id="net-broker" placeholder="voicevon.vicp.io"></div>
        <div class="fg"><label>MQTT 端口</label><input type="number" id="net-port" placeholder="1883" min="1" max="65535"></div>
      </div>
      <button class="btn primary" onclick="saveNetwork()">保存配置</button>
    </div>
    <div class="card">
      <div class="card-title">附近 WiFi 网络</div>
      <button class="btn" onclick="scanWifi()" style="margin-bottom:.8rem">扫描 WiFi</button>
      <div class="wifi-list" id="wifi-list"></div>
    </div>
  </div>
  <div class="footer">
    <p>AP: AP_THERMAL | 协议兼容: water_sensor_hx711 (FengBLE / MQTT / Web)</p>
    <p>版权所有: 山东卷积分公司</p>
  </div>
</main>
<div id="toast"></div>
<script>
const CH_NAMES=['CH1 碳源','CH2 次氯酸钠','CH3 聚合氯化铁'];
const CH_SUBS=['碳源','次氯酸钠','聚合氯化铁'];
const ST_TEXT={0:'空管',1:'有液',2:'首轮采集',3:'探头故障'};
const ST_CLS={0:'idle',1:'triggered',2:'warn',3:'fault'};
let cfgBuilt=false,tblBuilt=false;
function showTab(id,btn){
  document.querySelectorAll('.tab-panel').forEach(p=>p.classList.remove('active'));
  document.querySelectorAll('.tab-item').forEach(b=>b.classList.remove('active'));
  document.getElementById('tab-'+id).classList.add('active');
  btn.classList.add('active');
  if(id==='network')loadNetwork();
}
let _tt;
function toast(msg,type='ok'){
  const el=document.getElementById('toast');
  el.textContent=msg;el.className='show '+type;
  clearTimeout(_tt);_tt=setTimeout(()=>el.className='',3000);
}
async function post(url,params){
  const r=await fetch(url,{method:'POST',body:new URLSearchParams(params)});
  const t=await r.text();if(!r.ok)throw new Error(t);return t;
}
function buildCards(sensors,activeCh){
  const g=document.getElementById('sg');
  if(!tblBuilt){
    const rows=[
      ['液位状态','st'],['冷却衰减比','rt'],['实时温度','tn'],['峰值温升','dt'],
      ['10s/30s降温','dr'],['起止温度','t0'],['原始值','rw'],['运行状态','ht']
    ];
    let h='<table class="cmp-tbl"><thead><tr><th class="lbl">参数</th>';
    sensors.forEach((_,i)=>{
      h+='<th id="th'+i+'"><span class="ch-id">CH'+(i+1)+'</span><span class="ch-sub" id="cs'+i+'">'+(CH_SUBS[i]||'')+'</span></th>';
    });
    h+='</tr></thead><tbody>';
    rows.forEach(([label,key])=>{
      h+='<tr><td class="lbl">'+label+'</td>';
      sensors.forEach((_,i)=>{
        h+='<td id="'+key+i+'"'+(key==='rt'?' class="dt-cell"':'')+'>--</td>';
      });
      h+='</tr>';
    });
    h+='</tbody></table>';
    g.innerHTML=h;
    tblBuilt=true;
  }
  sensors.forEach((s,i)=>{
    const isAct=(activeCh===i);
    const heatCls=s.heating?' ch-heat':'';
    document.getElementById('th'+i).className=(isAct?'ch-active':'')+heatCls;
    document.getElementById('cs'+i).textContent=(CH_SUBS[i]||'')+'·'+(s.probe_type===1?'陶瓷':'不锈钢');
    document.getElementById('st'+i).className=heatCls.trim();
    document.getElementById('st'+i).innerHTML='<span class="badge '+(ST_CLS[s.state]||'idle')+'">'+(ST_TEXT[s.state]||'--')+'</span>';
    const rtEl=document.getElementById('rt'+i);
    rtEl.className='dt-cell'+(s.state===1?' liquid':(s.state===3?' fault':''))+heatCls;
    const thrHint='<span class="sub-hint">&gt;'+Math.round(s.ratio_liquid*100)+'% / &lt;'+Math.round(s.ratio_empty*100)+'%</span>';
    rtEl.innerHTML=(s.calibrated?(s.cool_ratio*100).toFixed(1)+'%':'--')+thrHint;
    const setCell=(k,txt)=>{const el=document.getElementById(k+i);el.className=heatCls.trim();el.textContent=txt;};
    setCell('tn',s.t_now===null?'--':Number(s.t_now).toFixed(2)+' ℃');
    const dtEl=document.getElementById('dt'+i);
    dtEl.className=heatCls.trim();
    dtEl.innerHTML='+'+Number(s.delta_t).toFixed(2)+' ℃'+(s.t_peak===null?'':'<span class="sub-hint">峰 '+Number(s.t_peak).toFixed(2)+'</span>');
    setCell('dr',Number(s.drop10).toFixed(2)+' / '+Number(s.drop30).toFixed(2));
    setCell('t0',(s.t0===null?'--':Number(s.t0).toFixed(1))+'→'+(s.t1===null?'--':Number(s.t1).toFixed(1)));
    setCell('rw',String(s.raw_val));
    const htEl=document.getElementById('ht'+i);
    htEl.className=(s.heating?'ht-on':'')+heatCls;
    htEl.textContent=s.heating?'🔥 加热中':(s.calibrating?'调参中':(isAct?'轮巡中':'待机'));
  });
}
function buildChannelConfigs(sensors){
  if(cfgBuilt)return;
  const list=document.getElementById('ch-cfg-list');
  list.innerHTML='';
  sensors.forEach((s,i)=>{
    list.insertAdjacentHTML('beforeend',
      '<div class="card">'+
      '<div class="card-title">'+(CH_NAMES[i]||('CH'+(i+1)))+' 封装与独立阈值</div>'+
      '<div class="fg"><label>探头封装类型（切换后自动应用推荐时序与默认阈值）</label>'+
      '<select id="pt'+i+'" onchange="saveProbeType('+i+')">'+
      '<option value="0"'+(s.probe_type===0?' selected':'')+'>0.5mm 不锈钢管（10s加热 / 0→10/30s衰减）</option>'+
      '<option value="1"'+(s.probe_type===1?' selected':'')+'>2.0mm 陶瓷管（30s加热 / 10→30/60s尾段衰减）</option>'+
      '</select></div>'+
      '<div class="form-row">'+
      '<div class="fg"><label>有液阈值 (衰减比 &gt;)</label><input type="number" id="rl'+i+'" step="0.02" min="0.20" max="0.90" value="'+s.ratio_liquid+'"></div>'+
      '<div class="fg"><label>空管阈值 (衰减比 &lt;)</label><input type="number" id="re'+i+'" step="0.02" min="0.15" max="0.85" value="'+s.ratio_empty+'"></div>'+
      '</div>'+
      '<div class="btn-row">'+
      '<button class="btn primary" onclick="saveRatios('+i+')">保存 CH'+(i+1)+' 阈值</button>'+
      '<button class="btn" onclick="doCal(\'start\','+i+')">单路空管调参</button>'+
      '<button class="btn danger" onclick="doCal(\'reset\','+i+')">恢复默认</button>'+
      '</div></div>');
  });
  cfgBuilt=true;
}
async function updateData(){
  try{
    const r=await fetch('/api/data');const d=await r.json();
    document.getElementById('dot-wifi').className='dot '+(d.wifi_connected?'on':'off');
    document.getElementById('dot-mqtt').className='dot '+(d.mqtt_connected?'on':'off');
    document.getElementById('bus-v5').textContent='5V: '+Number(d.bus_v5).toFixed(2)+'V';
    document.getElementById('cal-banner').textContent=d.calibrating?'正在执行空管标定中...':'当前未在标定（轮巡通道 CH'+(d.active_ch+1)+'）';
    document.getElementById('mon-active').textContent='CH'+(d.active_ch+1)+' '+(CH_SUBS[d.active_ch]||'');
    document.getElementById('mon-cal').textContent=d.calibrating?'⚙️ 空管标定中...':'1s 自动刷新';
    buildCards(d.sensors,d.active_ch);
    buildChannelConfigs(d.sensors);
  }catch(e){}
}
async function doCal(act,ch){
  try{const msg=await post('/api/calibrate',{action:act,ch:ch});toast(msg);updateData();}
  catch(e){toast('操作失败: '+e.message,'err');}
}
async function saveProbeType(ch){
  try{
    const pt=document.getElementById('pt'+ch).value;
    await post('/api/thermal',{ch:ch,probe_type:pt});
    cfgBuilt=false;
    await updateData();
    toast('CH'+(ch+1)+' 已切换为'+(pt==='1'?' 2mm 陶瓷管':' 0.5mm 不锈钢管'));
  }catch(e){toast('切换失败: '+e.message,'err');}
}
async function saveRatios(ch){
  try{
    await post('/api/thermal',{
      ch:ch,
      ratio_liquid:document.getElementById('rl'+ch).value,
      ratio_empty:document.getElementById('re'+ch).value
    });
    toast('CH'+(ch+1)+' 阈值已保存');
  }catch(e){toast('保存失败: '+e.message,'err');}
}
async function loadNetwork(){
  try{
    const r=await fetch('/api/sysconfig?t='+Date.now());const d=await r.json();
    document.getElementById('net-ssid').value=d.ssid||'';
    document.getElementById('net-name').value=d.name||'';
    document.getElementById('net-broker').value=d.broker||'';
    document.getElementById('net-port').value=d.port||1883;
    const p=document.getElementById('net-pass');
    p.value='';p.placeholder='留空表示保持不变';
  }catch(e){toast('加载配置失败','err');}
}
async function saveNetwork(){
  const params={
    ssid:document.getElementById('net-ssid').value,
    name:document.getElementById('net-name').value,
    broker:document.getElementById('net-broker').value,
    port:document.getElementById('net-port').value
  };
  const pw=document.getElementById('net-pass').value;
  if(pw)params.password=pw;
  try{
    await post('/api/sysconfig',params);toast('配置已保存并应用');
  }catch(e){toast('保存失败: '+e.message,'err');}
}
async function scanWifi(){
  const list=document.getElementById('wifi-list');
  list.innerHTML='<span style="color:var(--muted);font-size:.82rem">正在扫描...</span>';
  try{
    const r=await fetch('/api/scan');const d=await r.json();
    list.innerHTML='';
    d.networks.forEach(n=>{
      const div=document.createElement('div');div.className='wi';
      div.innerHTML='<span>'+n.ssid+'</span><span class="rs">'+n.rssi+' dBm</span>';
      div.onclick=()=>{document.getElementById('net-ssid').value=n.ssid;};
      list.appendChild(div);
    });
    if(!d.networks.length)list.innerHTML='<span style="color:var(--muted)">未发现网络</span>';
  }catch(e){list.innerHTML='<span style="color:var(--red)">扫描失败</span>';}
}
updateData();
setInterval(updateData,1000);
</script>
</body>
</html>
)rawhtml";
