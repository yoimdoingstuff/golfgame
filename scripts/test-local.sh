#!/usr/bin/env sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BIN="$ROOT/build/tg_net_demo"
HOST_LOG="${TMPDIR:-/tmp}/tg-host-smoke.log"
CLIENT_LOG="${TMPDIR:-/tmp}/tg-client-smoke.log"
"$BIN" host SmokeHost "Smoke Test" >"$HOST_LOG" 2>&1 &
HOST_PID=$!
cleanup() { kill "$HOST_PID" 2>/dev/null || true; }
trap cleanup EXIT INT TERM
sleep 0.3
"$BIN" join 127.0.0.1 SmokeClient >"$CLIENT_LOG" 2>&1 &
CLIENT_PID=$!
cleanup_all() { kill "$CLIENT_PID" 2>/dev/null || true; cleanup; }
trap cleanup_all EXIT INT TERM
sleep 1.5
if ! grep -q 'players 2/16' "$HOST_LOG"; then
  echo "Host did not register the client. Host log:" >&2
  cat "$HOST_LOG" >&2
  echo "Client log:" >&2
  cat "$CLIENT_LOG" >&2
  exit 1
fi
if ! grep -q 'SmokeClient' "$HOST_LOG"; then
  echo "Host did not receive the client's name." >&2
  cat "$HOST_LOG" >&2
  exit 1
fi
echo "PASS: localhost UDP host/join and 2-player snapshot"
