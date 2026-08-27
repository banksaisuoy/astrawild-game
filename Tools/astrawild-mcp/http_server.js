// Copyright Epic Games, Inc. / ASTRAWILD Team. All Rights Reserved.
// HTTP / SSE Bridge for ASTRAWILD Unreal Engine 5.8 MCP Server

const http = require('http');
const path = require('path');
const { spawn } = require('child_process');

const PORT = process.env.PORT || 3000;
const SERVER_JS = path.join(__dirname, 'server.js');

const server = http.createServer((req, res) => {
  // Enable CORS
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');

  if (req.method === 'OPTIONS') {
    res.writeHead(200);
    res.end();
    return;
  }

  if (req.method === 'GET' && (req.url === '/' || req.url === '/health')) {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ status: 'ok', server: 'astrawild-unreal-mcp', version: '1.0.0' }));
    return;
  }

  if (req.method === 'POST' && (req.url === '/mcp' || req.url === '/rpc' || req.url === '/')) {
    let body = '';
    req.on('data', chunk => { body += chunk.toString(); });
    req.on('end', () => {
      try {
        const proc = spawn('node', [SERVER_JS]);
        let output = '';
        let errOutput = '';

        proc.stdout.on('data', data => { output += data.toString(); });
        proc.stderr.on('data', data => { errOutput += data.toString(); });

        proc.on('close', code => {
          res.writeHead(200, { 'Content-Type': 'application/json' });
          // If output is JSON-RPC line, return it directly
          const lines = output.split('\n').filter(l => l.trim().startsWith('{'));
          if (lines.length > 0) {
            res.end(lines[lines.length - 1]);
          } else {
            res.end(JSON.stringify({ jsonrpc: '2.0', error: { code: -32603, message: errOutput || 'Internal execution error' } }));
          }
        });

        proc.stdin.write(body + '\n');
        proc.stdin.end();
      } catch (err) {
        res.writeHead(500, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ error: err.message }));
      }
    });
    return;
  }

  res.writeHead(404, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({ error: 'Not Found' }));
});

server.listen(PORT, '0.0.0.0', () => {
  console.log(`ASTRAWILD MCP HTTP Bridge running at http://localhost:${PORT}/mcp`);
});