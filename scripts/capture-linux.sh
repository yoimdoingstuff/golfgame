#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
LOG_DIR="$ROOT/logs"
SHOT_DIR="$LOG_DIR/screenshots"
GAME="$BUILD_DIR/golfgame"

mkdir -p "$SHOT_DIR"
rm -f "$SHOT_DIR"/*.png "$LOG_DIR/capture-report.txt" "$LOG_DIR/runtime-stdout.txt" "$LOG_DIR/runtime-stderr.txt"
REPORT="$LOG_DIR/capture-report.txt"
STDOUT="$LOG_DIR/runtime-stdout.txt"
STDERR="$LOG_DIR/runtime-stderr.txt"

{
    echo "Started: $(date --iso-8601=seconds)"
    echo "Game: $GAME"
    echo "DISPLAY: ${DISPLAY:-<unset>}"
    echo "LIBGL_ALWAYS_SOFTWARE: ${LIBGL_ALWAYS_SOFTWARE:-<unset>}"
    echo
    glxinfo -B 2>&1 || true
} > "$REPORT"

if [[ ! -x "$GAME" ]]; then
    echo "Game executable missing or not executable: $GAME" | tee -a "$REPORT"
    exit 1
fi

(
    cd "$BUILD_DIR"
    exec ./golfgame
) >"$STDOUT" 2>"$STDERR" &
APP_PID=$!
APP_RUNNING=1

cleanup() {
    if [[ "$APP_RUNNING" -eq 1 ]] && kill -0 "$APP_PID" 2>/dev/null; then
        kill -TERM "$APP_PID" 2>/dev/null || true
        for _ in $(seq 1 30); do
            kill -0 "$APP_PID" 2>/dev/null || break
            sleep 0.1
        done
        kill -KILL "$APP_PID" 2>/dev/null || true
        wait "$APP_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

WINDOW_ID=""
for _ in $(seq 1 120); do
    WINDOW_ID="$(xdotool search --onlyvisible --name 'Golfgame' 2>/dev/null | head -n 1 || true)"
    if [[ -n "$WINDOW_ID" ]]; then
        break
    fi
    if ! kill -0 "$APP_PID" 2>/dev/null; then
        set +e
        wait "$APP_PID"
        APP_EXIT=$?
        set -e
        APP_RUNNING=0
        echo "Game exited before creating a window. Exit code: $APP_EXIT" | tee -a "$REPORT"
        break
    fi
    sleep 0.25
done

if [[ -z "$WINDOW_ID" ]]; then
    {
        echo "No visible window titled Golfgame was found for PID $APP_PID."
        echo
        echo "X11 window list:"
        xdotool search --onlyvisible --name '.*' 2>&1 || true
        echo
        echo "Runtime stdout:"
        cat "$STDOUT" || true
        echo
        echo "Runtime stderr:"
        cat "$STDERR" || true
    } | tee -a "$REPORT"
    exit 1
fi

{
    echo "Game PID: $APP_PID"
    echo "Window ID: $WINDOW_ID"
    echo "Window title: $(xdotool getwindowname "$WINDOW_ID" 2>&1 || true)"
    xdotool getwindowgeometry --shell "$WINDOW_ID" 2>&1 || true
} >> "$REPORT"

xdotool windowmap "$WINDOW_ID" 2>/dev/null || true
timeout 10s xdotool windowfocus --sync "$WINDOW_ID"
sleep 2

capture() {
    local name="$1"
    local path="$SHOT_DIR/$name"
    local dimensions bytes mean attempt
    for attempt in $(seq 1 12); do
        if ! kill -0 "$APP_PID" 2>/dev/null; then
            echo "Game process exited before capture $name." | tee -a "$REPORT"
            return 1
        fi
        if ! xdotool getwindowname "$WINDOW_ID" >/dev/null 2>&1; then
            echo "Game window disappeared before capture $name." | tee -a "$REPORT"
            return 1
        fi
        timeout 15s xwd -silent -id "$WINDOW_ID" -out "$path.xwd"
        timeout 15s convert "$path.xwd" "$path"
        rm -f "$path.xwd"
        dimensions="$(identify -format '%wx%h' "$path")"
        bytes="$(stat -c '%s' "$path")"
        mean="$(convert "$path" -colorspace gray -format '%[fx:mean]' info:)"
        if [[ "$bytes" -ge 5000 ]] && awk -v value="$mean" 'BEGIN { exit !(value > 0.005) }'; then
            echo "$name | $dimensions | $bytes bytes | mean=$mean | attempt=$attempt | $(date --iso-8601=seconds)" | tee -a "$REPORT"
            return 0
        fi
        echo "Retrying $name: blank/early frame ($bytes bytes, mean=$mean), attempt $attempt/12" | tee -a "$REPORT"
        sleep 1
    done
    echo "No rendered frame was captured for $name after 12 attempts." | tee -a "$REPORT"
    return 1
}

click_at() {
    timeout 10s xdotool mousemove --window "$WINDOW_ID" "$1" "$2" click 1
    sleep 0.4
}

capture "00-startup.png"
sleep 6
capture "01-main-menu.png"

# Host a real local room from the new front-end multiplayer panel.
click_at 1005 178
sleep 1
capture "02-host-lobby.png"

# Open the upstream course selector.
click_at 640 540
sleep 1
capture "03-course-select.png"

# Select the first course tile in the default course-picker layout.
click_at 424 246
sleep 6
capture "04-course-gameplay.png"

# Open the debug console, then choose the Golfgame Multiplayer tab.
timeout 10s xdotool key --window "$WINDOW_ID" grave
sleep 1
capture "05-debug-console.png"
click_at 225 48
capture "06-multiplayer-tab.png"

COUNT="$(find "$SHOT_DIR" -maxdepth 1 -type f -name '*.png' | wc -l)"
echo "Captured $COUNT PNG screenshots." | tee -a "$REPORT"
if [[ "$COUNT" -lt 4 ]]; then
    echo "Expected at least four visual checkpoints." | tee -a "$REPORT"
    exit 1
fi
