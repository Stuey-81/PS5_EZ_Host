// PS5_EZ_Host – no-cache savepoint + onboard RGB rainbow
// Board: ESP32-S3 (Arduino core) – uses WebServer, SPIFFS, WiFi softAP @ 10.1.1.1

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <Adafruit_NeoPixel.h>

WebServer server(80);

// ===== Onboard RGB config (adjust RGB_PIN if needed: try 48 -> 2 -> 38) =====
#define RGB_PIN        48
#define RGB_PIXELS     1
#define RGB_BRIGHTNESS 18

Adafruit_NeoPixel EZ_RGB(RGB_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);
static uint32_t rgb_last_ms = 0;
static uint16_t rgb_hue = 0;

static uint32_t rgb_wheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85)  return EZ_RGB.Color(255 - pos * 3, 0, pos * 3);
  if (pos < 170) { pos -= 85; return EZ_RGB.Color(0, pos * 3, 255 - pos * 3); }
  pos -= 170;    return EZ_RGB.Color(pos * 3, 255 - pos * 3, 0);
}
static void rgb_begin() {
  EZ_RGB.begin();
  EZ_RGB.setBrightness(RGB_BRIGHTNESS);
  EZ_RGB.clear();
  EZ_RGB.show();
}
static void rgb_tick() {
  uint32_t now = millis();
  if (now - rgb_last_ms < 12) return; // ~83 FPS, non-blocking
  rgb_last_ms = now;
  EZ_RGB.setPixelColor(0, rgb_wheel((byte)(rgb_hue & 0xFF)));
  EZ_RGB.show();
  rgb_hue++;
}

// ===== No-cache headers =====
static void addNoCacheHeaders() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
}

// ===== Utilities =====
static String contentTypeFor(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css"))  return "text/css";
  if (path.endsWith(".js"))   return "application/javascript";
  if (path.endsWith(".png"))  return "image/png";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
  if (path.endsWith(".gif"))  return "image/gif";
  if (path.endsWith(".svg"))  return "image/svg+xml";
  if (path.endsWith(".ico"))  return "image/x-icon";
  if (path.endsWith(".appcache")) return "text/cache-manifest";
  return "application/octet-stream";
}
static String baseName(const String& p) {
  int slash = p.lastIndexOf('/');
  return (slash >= 0) ? p.substring(slash + 1) : p;
}
static int hexVal(char c){ if(c>='0'&&c<='9')return c-'0'; if(c>='A'&&c<='F')return c-'A'+10; if(c>='a'&&c<='f')return c-'a'+10; return -1; }
static String urlDecode(const String& s){
  String out; out.reserve(s.length());
  for (size_t i=0;i<s.length();++i){
    char c=s[i];
    if (c=='%' && i+2<s.length()){
      int hi=hexVal(s[i+1]), lo=hexVal(s[i+2]);
      if (hi>=0 && lo>=0){ out += char((hi<<4)|lo); i+=2; continue; }
    } else if (c=='+') { out+=' '; continue; }
    out+=c;
  }
  return out;
}
static String urlEncode(const String& s){
  const char *hex="0123456789ABCDEF";
  String out; out.reserve(s.length()*3);
  for (size_t i=0;i<s.length();++i){
    char c=s[i];
    bool safe=(c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~'||c=='/';
    if (safe) out+=c; else { out+='%'; out+=hex[(c>>4)&0xF]; out+=hex[c&0xF]; }
  }
  return out;
}
static bool tryOpen(const String& path, File& out) {
  if (!SPIFFS.exists(path)) return false;
  out = SPIFFS.open(path, "r");
  return (bool)out;
}
static bool tryOpenCandidates(const String& reqRaw, File& out, String& chosen) {
  String req = urlDecode(reqRaw);
  if (!req.startsWith("/")) req = "/" + req;
  String base = baseName(req);

  if (tryOpen(req, out)) { chosen = req; return true; }

  String noslash = req.startsWith("/") ? req.substring(1) : req;
  if (noslash != req && tryOpen(noslash, out)) { chosen = noslash; return true; }

  String rootPath = "/" + base;
  if (rootPath != req && tryOpen(rootPath, out)) { chosen = rootPath; return true; }

  if (base != noslash && tryOpen(base, out)) { chosen = base; return true; }

  String payloadPath = "/payloads/" + base;
  if (payloadPath != req && tryOpen(payloadPath, out)) { chosen = payloadPath; return true; }

  String payloadNo = "payloads/" + base;
  if (payloadNo != noslash && tryOpen(payloadNo, out)) { chosen = payloadNo; return true; }

  return false;
}
static void streamFileNoCache(const String& path) {
  if (!SPIFFS.exists(path)) { addNoCacheHeaders(); server.send(404, "text/plain", "File Not Found"); return; }
  File f = SPIFFS.open(path, "r");
  addNoCacheHeaders();
  server.streamFile(f, contentTypeFor(path));
  f.close();
}

// ===== Routes =====
void handleFileRequest() {
  String path = server.uri();
  if (path == "/") path = "/index.html";
  streamFileNoCache(path);
}

void handleAdmin() {
  String html;
  html.reserve(4096);
  html += F("<!doctype html><html><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width,initial-scale=1'>"
            "<title>EZ Host Admin (No-Cache)</title>"
            "<style>body{font-family:sans-serif;background:#111;color:#eee;padding:12px}"
            "table{border-collapse:collapse;width:100%;background:#1b1b1b}"
            "th,td{border:1px solid #333;padding:6px;text-align:left}"
            "a{color:#8df}button{padding:6px 10px;margin-left:6px;cursor:pointer}</style>"
            "</head><body><h2>EZ Host Admin (No-Cache)</h2>");

  size_t total = SPIFFS.totalBytes();
  size_t used  = SPIFFS.usedBytes();
  html += "<p style='opacity:.7'>SPIFFS: used " + String(used) + " / " + String(total) + " (free " + String(total - used) + ")</p>";

  html += F("<form method='POST' action='/upload' enctype='multipart/form-data'>"
            "<input type='file' name='file'>"
            "<input type='submit' value='Upload'></form>");

  html += F("<table><thead><tr><th>Name</th><th>Size</th><th>Path</th><th>Actions</th></tr></thead><tbody>");

  if (SPIFFS.exists("/payload_map.js")) {
    File mf = SPIFFS.open("/payload_map.js", "r");
    html += "<tr><td>payload_map.js</td><td>" + String(mf.size()) + "</td><td>/payload_map.js</td><td>";
    String q = urlEncode("/payload_map.js");
    html += "<a href='/download?file=" + q + "'>Download</a> ";
    html += "<a href='/delete?file=" + q + "' onclick='return confirm(\"Delete payload_map.js?\")'>Delete</a>";
    html += "</td></tr>";
    mf.close();
  }

  File root = SPIFFS.open("/");
  for (File f = root.openNextFile(); f; f = root.openNextFile()) {
    if (f.isDirectory()) continue;
    String p = f.name();
    if (p.length() == 0) continue;
    int dot = p.lastIndexOf('.');
    if (dot < 0) continue;
    String ext = p.substring(dot);
    if (ext != ".elf" && ext != ".bin") continue;

    if (!p.startsWith("/")) p = "/" + p;
    String base = baseName(p);
    String q = urlEncode(p);

    html += "<tr><td>" + base + "</td><td>" + String(f.size()) + "</td><td>" + p + "</td><td>";
    html += "<a href='/download?file=" + q + "'>Download</a> ";
    html += "<a href='/delete?file=" + q + "' onclick='return confirm(\"Delete " + p + "?\")'>Delete</a>";
    html += "</td></tr>";
  }

  html += F("</tbody></table>"
            "<p style='opacity:.7'>No-cache mode: every request is served fresh; AppCache disabled.</p>"
            "</body></html>");
  addNoCacheHeaders();
  server.send(200, "text/html", html);
}

// ---- Upload (streams to /payloads except payload_map.js) ----
static File gUpFile;
static String gUpTarget;
static size_t gUpWritten = 0;

void handleUpload() {
  HTTPUpload& up = server.upload();

  if (up.status == UPLOAD_FILE_START) {
    String base = up.filename;
    if (base.startsWith("/")) base.remove(0,1);
    gUpTarget = (base == "payload_map.js") ? "/payload_map.js" : ("/payloads/" + base);

    size_t total = SPIFFS.totalBytes(), used = SPIFFS.usedBytes();
    Serial.printf("[UPLOAD] start -> %s (free=%u)\n", gUpTarget.c_str(), (unsigned)(total - used));

    SPIFFS.remove(gUpTarget);
    gUpFile = SPIFFS.open(gUpTarget, "w");
    gUpWritten = 0;
    if (!gUpFile) Serial.println("[UPLOAD] ERROR: open for write failed");

  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (gUpFile) {
      size_t w = gUpFile.write(up.buf, up.currentSize);
      gUpWritten += w;
      if (w != up.currentSize) Serial.println("[UPLOAD] WARNING: short write (likely out of space)");
    }

  } else if (up.status == UPLOAD_FILE_END) {
    if (gUpFile) gUpFile.close();
    Serial.printf("[UPLOAD] end   -> %s (total=%u)\n", gUpTarget.c_str(), (unsigned)gUpWritten);
  }
}

// ---- Delete (tolerant paths) ----
void handleDelete() {
  if (!server.hasArg("file")) { addNoCacheHeaders(); server.send(400, "text/plain", "No file specified"); return; }
  String arg = urlDecode(server.arg("file")); arg.trim();
  String path = arg.startsWith("/") ? arg : "/" + arg;
  String base = baseName(path);
  bool ok = false;

  if (SPIFFS.exists(path)) ok = SPIFFS.remove(path);
  if (!ok) {
    String rootPath = "/" + base;
    if (SPIFFS.exists(rootPath)) ok = SPIFFS.remove(rootPath);
  }
  if (!ok) {
    String alt = "/payloads/" + base;
    if (SPIFFS.exists(alt)) ok = SPIFFS.remove(alt);
  }

  Serial.printf("[DELETE] req=%s -> %s\n", arg.c_str(), ok ? "OK" : "MISS");
  addNoCacheHeaders();
  server.sendHeader("Location", "/admin", true);
  server.send(303);
}

// ---- Download (manual stream) ----
void handleDownload() {
  if (!server.hasArg("file")) { addNoCacheHeaders(); server.send(400, "text/plain", "No file specified"); return; }
  String reqRaw = server.arg("file"); reqRaw.trim();

  File f;
  String chosen;
  if (!tryOpenCandidates(reqRaw, f, chosen)) {
    addNoCacheHeaders();
    server.send(404, "text/plain", "File not found");
    Serial.printf("[DOWNLOAD] MISS: %s\n", reqRaw.c_str());
    return;
  }

  String fname = baseName(chosen);
  WiFiClient client = server.client();

  addNoCacheHeaders();
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + fname + "\"");
  server.setContentLength(f.size());
  server.send(200, "application/octet-stream", "");  // headers only

  uint8_t buf[1460];
  size_t total = 0;
  while (f.available() && client.connected()) {
    size_t n = f.read(buf, sizeof(buf));
    if (!n) break;
    size_t w = client.write(buf, n);
    total += w;
    delay(0);
  }
  f.close();
  client.flush();
  Serial.printf("[DOWNLOAD] HIT: %s -> %s (sent=%u)\n", reqRaw.c_str(), chosen.c_str(), (unsigned)total);
}

// ---- AppCache disabled ----
void handleNoCacheManifest() {
  String man = "CACHE MANIFEST\n# nocache-";
  man += String(millis());
  man += "\n\nNETWORK:\n*\n";
  addNoCacheHeaders();
  server.send(200, "text/cache-manifest", man);
}

// ---- Diagnostics ----
void handleRaw() {
  if (!server.hasArg("file")) { addNoCacheHeaders(); server.send(400, "text/plain", "No file specified"); return; }
  String req = urlDecode(server.arg("file")); req.trim();
  if (!req.startsWith("/")) req = "/" + req;
  if (!SPIFFS.exists(req)) { addNoCacheHeaders(); server.send(404, "text/plain", "Not Found: " + req); return; }
  streamFileNoCache(req);
}
void handleExists() {
  if (!server.hasArg("file")) { addNoCacheHeaders(); server.send(400, "text/plain", "No file specified"); return; }
  String req = urlDecode(server.arg("file")); req.trim();
  if (!req.startsWith("/")) req = "/" + req;
  String base = baseName(req);

  String out;
  out += "req=" + req + "\n";
  out += "exists(req)=" + String(SPIFFS.exists(req)) + "\n";
  String rootPath = "/" + base;
  out += "exists(" + rootPath + ")=" + String(SPIFFS.exists(rootPath)) + "\n";
  String payloadPath = "/payloads/" + base;
  out += "exists(" + payloadPath + ")=" + String(SPIFFS.exists(payloadPath)) + "\n";
  addNoCacheHeaders();
  server.send(200, "text/plain", out);
}
void handleFsInfo() {
  size_t total = SPIFFS.totalBytes();
  size_t used  = SPIFFS.usedBytes();
  String out = "total=" + String(total) + "\nused=" + String(used) + "\nfree=" + String(total - used) + "\n";
  addNoCacheHeaders();
  server.send(200, "text/plain", out);
}
void handleNotFound() { handleFileRequest(); }

// ===== Setup / Loop =====
void setup() {
  Serial.begin(115200);
  delay(50);
  bool spiffs_ok = SPIFFS.begin(true);
  Serial.println(spiffs_ok ? "SPIFFS OK" : "SPIFFS FAIL");

  // Wi-Fi AP @ 10.1.1.1
  WiFi.softAPConfig(IPAddress(10,1,1,1), IPAddress(10,1,1,1), IPAddress(255,255,255,0));
  WiFi.softAP("PS5_EZ_Host", "");
  Serial.print("IP address: "); Serial.println(WiFi.softAPIP());

  // Routes
  server.on("/", HTTP_GET, handleFileRequest);
  server.on("/admin", HTTP_GET, handleAdmin);

  server.on("/upload", HTTP_POST,
            [](){ addNoCacheHeaders(); server.sendHeader("Location","/admin",true); server.send(303); },
            handleUpload);

  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/download", HTTP_GET, handleDownload);

  server.on("/cache.appcache", HTTP_GET, handleNoCacheManifest);

  // Diags
  server.on("/raw", HTTP_GET, handleRaw);
  server.on("/exists", HTTP_GET, handleExists);
  server.on("/fsinfo", HTTP_GET, handleFsInfo);
  server.on("/ls", HTTP_GET, [](){
    String out;
    File root = SPIFFS.open("/");
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
      out += String(f.isDirectory() ? "D " : "F ") + String(f.name()) + " (" + String(f.size()) + ")\n";
    }
    addNoCacheHeaders();
    server.send(200, "text/plain", out);
  });

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Server started (no-cache mode)");

  // Start RGB
  rgb_begin();
}

void loop() {
  server.handleClient();
  rgb_tick(); // animate LED without blocking anything
}
