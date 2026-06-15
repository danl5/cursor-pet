#include "pet_server.h"
#include "settings.h"
#include <ArduinoJson.h>

extern bool inAPMode;

static const char CONFIG_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Cursor Pet Setup</title>
<style>
body{font-family:system-ui,-apple-system,sans-serif;max-width:400px;margin:40px auto;padding:0 20px;background:#1a1a2e;color:#e0e0e0}
h1{color:#f0c040;font-size:1.5em;text-align:center}
.status{background:#16213e;border-radius:8px;padding:12px;margin:16px 0;font-size:0.9em}
.status .label{color:#888}
.status .value{color:#4ecca3;font-weight:bold}
.form-group{margin:16px 0}
label{display:block;margin-bottom:4px;color:#aaa;font-size:0.85em}
input[type=text],input[type=password]{width:100%;padding:10px;border:1px solid #333;border-radius:6px;background:#0f3460;color:#fff;font-size:1em;box-sizing:border-box}
button{width:100%;padding:12px;border:none;border-radius:6px;font-size:1em;font-weight:bold;cursor:pointer;margin-top:8px}
.btn-save{background:#4ecca3;color:#1a1a2e}
.btn-reboot{background:#e94560;color:#fff;margin-top:12px}
.btn-save:active{background:#3ba882}
.hint{color:#666;font-size:0.75em;margin-top:4px}
</style>
</head>
<body>
<h1>Cursor Pet Setup</h1>
<div class="status">
<div><span class="label">Status: </span><span class="value" id="status">Loading...</span></div>
<div><span class="label">SSID: </span><span class="value" id="ssid">-</span></div>
<div><span class="label">IP: </span><span class="value" id="ip">-</span></div>
</div>
<div class="form-group">
<label>WiFi SSID</label>
<input type="text" id="inputSsid" placeholder="Enter WiFi name">
</div>
<div class="form-group">
<label>WiFi Password</label>
<input type="password" id="inputPass" placeholder="Enter WiFi password">
<div class="hint">Leave empty for open network</div>
</div>
<div class="form-group">
<label>Identity (Enterprise WiFi)</label>
<input type="text" id="inputIdentity" placeholder="Leave empty for personal WiFi">
<div class="hint">For WPA2-Enterprise networks (e.g. corporate WiFi). Identity is usually your username.</div>
</div>
<button class="btn-save" onclick="saveConfig()">Save & Reboot</button>
<button class="btn-reboot" onclick="reboot()">Reboot Device</button>
<script>
fetch('/api/config').then(r=>r.json()).then(d=>{
document.getElementById('status').textContent=d.connected?'Connected':'Not connected';
document.getElementById('ssid').textContent=d.ssid||'(none)';
document.getElementById('ip').textContent=d.ip||'-';
if(d.ssid)document.getElementById('inputSsid').placeholder=d.ssid;
if(d.identity)document.getElementById('inputIdentity').placeholder=d.identity;
});
function saveConfig(){
var s=document.getElementById('inputSsid').value;
var p=document.getElementById('inputPass').value;
var i=document.getElementById('inputIdentity').value;
if(!s){alert('Please enter WiFi SSID');return;}
fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},
body:JSON.stringify({ssid:s,password:p,identity:i})})
.then(r=>r.json()).then(d=>{
if(d.status==='ok'){alert('Saved! Device will reboot.');setTimeout(()=>location.reload(),3000);}
else alert('Error: '+d.error);
});
}
function reboot(){
if(confirm('Reboot device?')){fetch('/api/reboot',{method:'POST'}).then(()=>alert('Rebooting...'));}
}
</script>
</body>
</html>
)rawliteral";

PetServer::PetServer(PixelPet& pet, uint16_t port)
    : _pet(pet), _server(port) {}

void PetServer::begin() {
    _server.on("/api/state", HTTP_POST, [this]() { _handleState(); });
    _server.on("/api/stats", HTTP_POST, [this]() { _handleStats(); });
    _server.on("/health", HTTP_GET, [this]() { _handleHealth(); });
    _server.on("/", HTTP_GET, [this]() { _handleRoot(); });
    _server.on("/config", HTTP_GET, [this]() { _handleConfigPage(); });
    _server.on("/api/config", HTTP_GET, [this]() { _handleConfigGet(); });
    _server.on("/api/config", HTTP_POST, [this]() { _handleConfigSave(); });
    _server.on("/api/reboot", HTTP_POST, [this]() { _handleReboot(); });
    _server.on("/api/growth", HTTP_GET, [this]() { _handleGrowth(); });
    _server.on("/api/growth", HTTP_POST, [this]() { _handleGrowthReset(); });
    _server.on("/api/activity", HTTP_POST, [this]() { _handleActivity(); });
    _server.on("/api/streak", HTTP_GET, [this]() { _handleStreak(); });
    _server.begin();
    Serial.println("HTTP server started");
}

void PetServer::handleClient() {
    _server.handleClient();
}

void PetServer::_handleState() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    const char* state = doc["state"] | "idle";
    PetState s = PET_IDLE;
    if (strcmp(state, "idle") == 0)        s = PET_IDLE;
    else if (strcmp(state, "thinking") == 0) s = PET_THINKING;
    else if (strcmp(state, "working") == 0)  s = PET_WORKING;
    else if (strcmp(state, "sleep") == 0)    s = PET_SLEEP;
    else if (strcmp(state, "error") == 0)    s = PET_ERROR;

    _pet.setState(s);
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void PetServer::_handleStats() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    int tokens = doc["tokens"] | 0;
    int tasks = doc["tasks"] | 0;
    int errors = doc["errors"] | 0;
    _pet.setStats(tokens, tasks, errors);
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void PetServer::_handleHealth() {
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void PetServer::_handleRoot() {
    if (inAPMode) {
        _handleConfigPage();
    } else {
        _server.send(200, "text/plain", "Cursor Pet v1.0");
    }
}

void PetServer::_handleConfigPage() {
    _server.send_P(200, "text/html", CONFIG_PAGE);
}

void PetServer::_handleConfigGet() {
    DeviceSettings& s = settingsGet();
    JsonDocument doc;
    doc["connected"] = (WiFi.status() == WL_CONNECTED);
    doc["ssid"] = s.hasConfig ? s.wifiSSID : "";
    doc["identity"] = s.hasConfig ? s.wifiIdentity : "";
    if (WiFi.status() == WL_CONNECTED) {
        doc["ip"] = WiFi.localIP().toString();
    } else if (inAPMode) {
        doc["ip"] = WiFi.softAPIP().toString();
    } else {
        doc["ip"] = "";
    }
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void PetServer::_handleConfigSave() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    const char* ssid = doc["ssid"] | "";
    const char* password = doc["password"] | "";
    const char* identity = doc["identity"] | "";

    if (strlen(ssid) == 0) {
        _server.send(400, "application/json", "{\"error\":\"ssid required\"}");
        return;
    }

    settingsSetWiFi(ssid, password, identity);
    Serial.printf("WiFi config saved: %s\n", ssid);
    _server.send(200, "application/json", "{\"status\":\"ok\"}");

    delay(500);
    ESP.restart();
}

void PetServer::_handleReboot() {
    _server.send(200, "application/json", "{\"status\":\"rebooting\"}");
    delay(500);
    ESP.restart();
}

void PetServer::_handleGrowth() {
    JsonDocument doc;
    const char* stages[] = {"baby", "kitten", "adult", "wizard"};
    doc["stage"] = stages[_pet.getGrowthStage()];
    doc["stageId"] = _pet.getGrowthStage();
    doc["totalTasks"] = _pet.getTotalTasks();
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void PetServer::_handleGrowthReset() {
    settingsResetGrowth();
    _pet.setGrowthData(0, 0);
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void PetServer::_handleActivity() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    int thoughts = doc["thoughts"] | 0;
    int tools = doc["tools"] | 0;
    _pet.addActivity(thoughts, tools);

    JsonDocument resp;
    resp["status"] = "ok";
    resp["totalThoughts"] = _pet.getActivityThoughts();
    resp["totalTools"] = _pet.getActivityTools();
    String json;
    serializeJson(resp, json);
    _server.send(200, "application/json", json);
}

void PetServer::_handleStreak() {
    DeviceSettings& s = settingsGet();
    JsonDocument doc;
    doc["streak"] = s.streakCount;
    doc["lastDay"] = s.streakLastDay;
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}
