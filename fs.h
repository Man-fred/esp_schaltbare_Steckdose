#ifndef FS_INO_H
#define FS_INO_H

#include "zcommon.h"
#include "log.h"

File fsUploadFile;
extern byte is_authentified();

String getContentType(String filename) {
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".manifest")) return "text/cache-manifest";
  return "text/plain";
}

bool handleFileRead(String path) {
  if (path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, contentType); // size_t sent = 
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload() {
  if (is_authentified()) {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      String filename = upload.filename;
      if (!filename.startsWith("/")) filename = "/" + filename;
      DBG_OUTPUT_PORT.print("handleFileUpload Start: "); DBG_OUTPUT_PORT.println(filename);
      fsUploadFile = SPIFFS.open(filename, "w");
      filename = String();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (fsUploadFile) {
        fsUploadFile.write(upload.buf, upload.currentSize);
        DBG_OUTPUT_PORT.print("handleFileUpload Data : "); DBG_OUTPUT_PORT.println(upload.currentSize);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (fsUploadFile) {
        fsUploadFile.close();
        DBG_OUTPUT_PORT.print("handleFileUpload End  : "); DBG_OUTPUT_PORT.println(upload.totalSize);
      }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        DBG_OUTPUT_PORT.println("handleFileUpload Aborted");
    }
  }
}

void handleFileDelete() {
  if (is_authentified()) {
    if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
    String path = server.arg(0);
    DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
    if (path == "/")
      return server.send(500, "text/plain", "BAD PATH");
    if (!SPIFFS.exists(path))
      return server.send(404, "text/plain", "FileNotFound");
    SPIFFS.remove(path);
    server.send(200, "text/plain", "");
    path = String();
  }
}

void handleFileCreate() {
  if (is_authentified()) {
    if (server.args() == 0)
      return server.send(500, "text/plain", "BAD ARGS");
    String path = server.arg(0);
    DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
    if (path == "/")
      return server.send(500, "text/plain", "BAD PATH");
    if (SPIFFS.exists(path))
      return server.send(500, "text/plain", "FILE EXISTS");
    File file = SPIFFS.open(path, "w");
    if (file)
      file.close();
    else
      return server.send(500, "text/plain", "CREATE FAILED");
    server.send(200, "text/plain", "");
    path = String();
  }
}

void handleFileList() {
  if (is_authentified()) {
    if (!server.hasArg("dir")) {
      server.send(500, "text/plain", "BAD ARGS");
      return;
    }

    String path = server.arg("dir");
    DBG_OUTPUT_PORT.println("handleFileList: " + path);
    Dir dir = SPIFFS.openDir(path);
    path = String();

    String output = "[";
    while (dir.next()) {
      File entry = dir.openFile("r");
      if (output != "[") output += ',';
      bool isDir = false;
      output += "{\"type\":\"";
      output += (isDir) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += String(entry.name()).substring(1);
      output += "\",\"size\":\"";
      output += String(entry.size());
      output += "\"}";
      entry.close();
    }

    output += "]";
    server.send(200, "text/json", output);
  }
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void updateVersion() {
  if (is_authentified()) {
    server.sendHeader("Location", "/index.htm");
    server.send(303, "text/html", ""); // Antwort an Internet Browser
    t_httpUpdate_return ret = ESPhttpUpdate.update(UpdateServer, 80, "/esp8266/ota.php", (mVersionNr + mVersionVariante + mVersionBoard).c_str());
  
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        LogSchreibenNow("[update] " + mVersionNr + mVersionVariante + mVersionBoard + " Update failed.");
        LogSchreibenNow(/*ESPhttpUpdate.getLastError() +" "+*/ ESPhttpUpdate.getLastErrorString().c_str());
        break;
      case HTTP_UPDATE_NO_UPDATES:
        DBG_OUTPUT_PORT.println("[update] " + mVersionNr + mVersionVariante + mVersionBoard + " No update.");
        break;
      case HTTP_UPDATE_OK:
        LogSchreibenNow("[update] " + mVersionNr + mVersionVariante + mVersionBoard + " Update ok."); // may not called, we reboot the ESP
        break;
      default:
        LogSchreibenNow("[update] " + mVersionNr + mVersionVariante + mVersionBoard + " unknown error");
        LogSchreibenNow(/*ESPhttpUpdate.getLastError() +" "+*/ ESPhttpUpdate.getLastErrorString().c_str());
        break;
    }
  }
}
#endif
