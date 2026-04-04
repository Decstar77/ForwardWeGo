"""
Simple HTTP server for the ForwardWeGo web build.

Usage:
    python serve_web.py [ship_folder]

If no folder is given, serves the most recent ship/*_web/ directory.
Opens the browser automatically. Press Ctrl+C to stop.
"""

import http.server
import os
import sys
import webbrowser
import glob

PORT = 8080

def find_latest_web_ship():
    dirs = sorted(glob.glob("ship/*_web"), reverse=True)
    if dirs:
        return dirs[0]
    return None

def main():
    if len(sys.argv) > 1:
        directory = sys.argv[1]
    else:
        directory = find_latest_web_ship()
        if directory is None:
            print("No ship/*_web/ directory found. Build the web version first or pass a path.")
            sys.exit(1)

    directory = os.path.abspath(directory)
    if not os.path.isfile(os.path.join(directory, "index.html")):
        print(f"Error: {directory} does not contain index.html")
        sys.exit(1)

    os.chdir(directory)

    handler = http.server.SimpleHTTPRequestHandler
    handler.extensions_map[".wasm"] = "application/wasm"
    handler.extensions_map[".data"] = "application/octet-stream"

    server = http.server.HTTPServer(("", PORT), handler)
    url = f"http://localhost:{PORT}"
    print(f"Serving {directory}")
    print(f"Open {url}")
    print("Press Ctrl+C to stop.")
    webbrowser.open(url)

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")
        server.server_close()

if __name__ == "__main__":
    main()
