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
sleep 2.5
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
if ! grep -q 'SmokeClient(hole=6,x=4.50,vx=-1.25,strokes=7)' "$HOST_LOG"; then
  echo "Host did not receive the client's mapped hole, position, velocity and stroke count." >&2
  cat "$HOST_LOG" >&2
  echo "Client log:" >&2
  cat "$CLIENT_LOG" >&2
  exit 1
fi
if ! grep -q 'SmokeHost(hole=2,x=1.25,vx=0.75,strokes=7)' "$CLIENT_LOG"; then
  echo "Client did not receive the host's mapped hole, position, velocity and stroke count." >&2
  cat "$CLIENT_LOG" >&2
  exit 1
fi
if ! grep -Eq 'RTT [0-9]+([.][0-9]+)? ms' "$HOST_LOG"; then
  echo "Host did not complete a round-trip latency measurement." >&2
  cat "$HOST_LOG" >&2
  exit 1
fi
if ! grep -Eq 'RTT [0-9]+([.][0-9]+)? ms' "$CLIENT_LOG"; then
  echo "Client did not complete a round-trip latency measurement." >&2
  cat "$CLIENT_LOG" >&2
  exit 1
fi
if ! grep -q 'MSG channel=77 sender=.* payload=client-to-host' "$HOST_LOG"; then
  echo "Host did not receive the reliable client-to-host application message." >&2
  cat "$HOST_LOG" >&2
  cat "$CLIENT_LOG" >&2
  exit 1
fi
if ! grep -q 'MSG channel=78 sender=.* payload=host-broadcast' "$CLIENT_LOG"; then
  echo "Client did not receive the reliable host broadcast application message." >&2
  cat "$HOST_LOG" >&2
  cat "$CLIENT_LOG" >&2
  exit 1
fi
echo "PASS: localhost UDP host/join, identity, hole index, position, velocity, strokes, RTT and reliable app messages in both directions"
