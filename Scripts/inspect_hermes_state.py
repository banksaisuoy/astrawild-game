import sqlite3
import json
import sys

sys.stdout.reconfigure(encoding='utf-8')

db_path = r"C:\Users\saisu\AppData\Local\hermes\state.db"
conn = sqlite3.connect(db_path)
cursor = conn.cursor()

cursor.execute("PRAGMA table_info(messages)")
print("Columns in messages:", [c[1] for c in cursor.fetchall()])

cursor.execute("""
SELECT id, session_id, role, content, tool_calls, tool_call_id
FROM messages
WHERE session_id = '20260829_103433_9dee3e'
ORDER BY rowid DESC LIMIT 25
""")

cols = [c[0] for c in cursor.description]
rows = cursor.fetchall()
rows.reverse()

for r in rows:
    d = dict(zip(cols, r))
    role = d.get('role')
    content = d.get('content') or ""
    tool_calls = d.get('tool_calls')
    tool_call_id = d.get('tool_call_id')
    
    print(f"\n--- [ROLE: {role}] (Tool ID: {tool_call_id}) ---")
    if tool_calls:
        print(f"[TOOL CALLS]: {tool_calls}")
    if content:
        if len(content) > 600:
            print(f"[CONTENT (truncated)]:\n{content[:600]}...\n[END TRUNCATED]")
        else:
            print(f"[CONTENT]:\n{content}")
