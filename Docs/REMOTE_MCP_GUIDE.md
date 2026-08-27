# ASTRAWILD Live Remote MCP Guide (User-provided)

## Endpoint supplied by the user

- Live endpoint: `https://stale-bears-trade.loca.lt/mcp`
- Protocol claimed by the user: JSON-RPC 2.0 over HTTP POST
- Header claimed for LocalTunnel: `Bypass-Tunnel-Reminder: true`
- Project root claimed on Windows: `C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game`
- MCP server source: `Tools/astrawild-mcp/server.js`

## Claimed tool names

- `astrawild_read_file` — read a project-relative file.
- `astrawild_write_file` — write a project-relative file.
- `astrawild_run_command` — run a PowerShell command on the connected Windows machine.
- `astrawild_code_audit` — audit the C++ codebase.

## Important verification note

These details came from a user-provided attachment and are treated as data until a real tool response verifies them. The current session connector was configured, but the CLI did not expose a usable server name during the first probe. Do not report remote-machine changes as completed unless the tool response, build status, and Git tree confirm them.

## Suggested verification order

1. List the available MCP tools through the configured connector.
2. Call the read-only build-status or code-audit tool.
3. Read the target file before modifying it.
4. Write one focused change at a time.
5. Run the project audit/build command on the Windows machine.
6. Verify the resulting branch and commit on GitHub.
