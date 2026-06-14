#include "server.h"
#include <ArduinoJson.h>

PetServer::PetServer(PixelPet& pet, uint16_t port)
    : _pet(pet), _server(port) {}

void PetServer::begin() {
    _server.on("/api/state", HTTP_POST, [this]() { _handleState(); });
    _server.on("/api/stats", HTTP_POST, [this]() { _handleStats(); });
    _server.on("/health", HTTP_GET, [this]() { _handleHealth(); });
    _server.on("/", HTTP_GET, [this]() { _handleRoot(); });
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

    StaticJsonDocument<128> doc;
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

    StaticJsonDocument<128> doc;
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
    _server.send(200, "text/plain", "Cursor Pet v1.0");
}
