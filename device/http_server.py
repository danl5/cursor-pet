import socket
import json


class HttpServer:
    def __init__(self, port=80):
        self.port = port
        self.server_socket = None
        self.handler = None

    def start(self):
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind(("0.0.0.0", self.port))
        self.server_socket.listen(3)
        self.server_socket.settimeout(0.1)

    def set_handler(self, handler):
        self.handler = handler

    def poll(self):
        if not self.server_socket:
            return
        try:
            client, addr = self.server_socket.accept()
            request = client.recv(1024).decode("utf-8")
            response = self._handle_request(request)
            client.sendall(response)
            client.close()
        except OSError:
            pass

    def _handle_request(self, request):
        try:
            method = request.split(" ")[0]
            path = request.split(" ")[1]

            if path == "/api/state" and method == "GET":
                return self._json_response(200, {"status": "ok", "state": "running"})

            if path == "/api/state" and method == "POST":
                body = self._extract_body(request)
                if self.handler:
                    self.handler("state", body)
                return self._json_response(200, {"status": "ok"})

            if path == "/api/pet" and method == "POST":
                body = self._extract_body(request)
                if self.handler:
                    self.handler("pet", body)
                return self._json_response(200, {"status": "ok"})

            if path == "/api/stats" and method == "POST":
                body = self._extract_body(request)
                if self.handler:
                    self.handler("stats", body)
                return self._json_response(200, {"status": "ok"})

            if path == "/health":
                return self._json_response(200, {"status": "ok"})

            return self._json_response(404, {"error": "not found"})

        except Exception as e:
            return self._json_response(500, {"error": str(e)})

    def _extract_body(self, request):
        try:
            parts = request.split("\r\n\r\n", 1)
            if len(parts) > 1:
                return json.loads(parts[1])
        except:
            pass
        return {}

    def _json_response(self, status, data):
        body = json.dumps(data)
        return (
            f"HTTP/1.1 {status} OK\r\n"
            "Content-Type: application/json\r\n"
            f"Content-Length: {len(body)}\r\n"
            "Connection: close\r\n"
            "\r\n"
            f"{body}"
        )
