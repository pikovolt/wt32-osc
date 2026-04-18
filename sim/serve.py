# serve.py
'''
使い方:
  'make wasm' で、WebSimulator向けのビルドを行う。
  'sim/' 階層に移動し、'python serve.py 8000' でwebサーバを起動
  ブラウザから 'localhost:8000/wt32_osc.html' を開く
'''
from http.server import SimpleHTTPRequestHandler
import socketserver, sys

class COEPHandler(SimpleHTTPRequestHandler):
    extensions_map = {
        '': 'application/octet-stream',
        '.html': 'text/html',
        '.js': 'application/javascript',
        '.wasm': 'application/wasm',
        '.json': 'application/json',
        '.png': 'image/png',
        '.css': 'text/css',
    }

    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        SimpleHTTPRequestHandler.end_headers(self)

if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    with socketserver.TCPServer(('', port), COEPHandler) as httpd:
        print(f'Serving on http://localhost:{port}')
        httpd.serve_forever()