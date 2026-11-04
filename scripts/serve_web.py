#!/usr/bin/env python3
"""
AetherDSP Interactive Web Visualizer Server
===========================================
Serves the 3D Cymatics and Non-Euclidean Hyperbolic Waveguide browser GUI.
Zero external dependencies (uses standard library http.server).
"""

import http.server
import socketserver
import webbrowser
import os
import sys

PORT = 8080
DIRECTORY = os.path.join(os.path.dirname(__file__), "..", "web")

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

def main():
    print(f"\033[1;36m=== Starting AetherDSP 3D Interactive Web Visualizer ===\033[0m")
    print(f"Serving directory: {os.path.abspath(DIRECTORY)}")
    print(f"URL: \033[1;32mhttp://localhost:{PORT}/\033[0m")
    print(f"Press Ctrl+C to terminate.")

    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        try:
            webbrowser.open(f"http://localhost:{PORT}/")
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer gracefully stopped.")

if __name__ == "__main__":
    main()
