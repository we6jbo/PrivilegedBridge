#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR="${1:-/home/we6jbo/Projects/fractionstutor}"
STAGE_ROOT="${XDG_CACHE_HOME:-$HOME/.cache}/privileged-bridge/snap-staging"
STAGE_DIR="$STAGE_ROOT/fractionstutor"
OUTPUT_DIR="$SOURCE_DIR"
BUILD_PID=""
TIMER_PID=""
START_TS="$(date +%s)"
AGENT_USER="${PRIVILEGED_BRIDGE_AGENT_USER:-mistral}"
REFERENCE_URL="https://j03.page/2026/09/19/giving-ai-safe-privileges/"

say() { printf '\n[%s] %s\n' "$1" "$2"; }
stop_timer() {
    if [[ -n "${TIMER_PID:-}" ]] && kill -0 "$TIMER_PID" 2>/dev/null; then
        kill "$TIMER_PID" 2>/dev/null || true
        wait "$TIMER_PID" 2>/dev/null || true
    fi
    TIMER_PID=""
}
cleanup_on_cancel() {
    stop_timer
    printf '\nCancel requested by user. Stopping Snapcraft...\n' >&2
    if [[ -n "${BUILD_PID:-}" ]] && kill -0 "$BUILD_PID" 2>/dev/null; then
        kill -INT "$BUILD_PID" 2>/dev/null || true
        sleep 2
        kill -TERM "$BUILD_PID" 2>/dev/null || true
    fi
    printf 'Build cancelled. Staging directory kept for inspection: %s\n' "$STAGE_DIR" >&2
    exit 130
}
trap cleanup_on_cancel INT TERM

if [[ ! -d "$SOURCE_DIR" ]]; then
    echo "Project not found: $SOURCE_DIR" >&2
    exit 1
fi
for cmd in rsync snapcraft unsquashfs; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "Required command not found: $cmd" >&2
        exit 1
    fi
done

printf 'PrivilegedBridge build_test_snap\n'
printf 'Agent identity: %s\n' "$AGENT_USER"
printf 'Project: %s\n' "$SOURCE_DIR"
printf 'PrivilegedBridge reference: %s\n' "$REFERENCE_URL"
printf 'Press Ctrl+C at any time during the build to request cancellation.\n'

say "1/6" "Preparing clean Snapcraft staging tree"
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR"
rsync -a --delete \
  --exclude='.git/' \
  --exclude='.flatpak-builder/' \
  --exclude='.privilegedbridge-backups/' \
  --exclude='build/' \
  --exclude='build-*/' \
  --exclude='build-dir*/' \
  --exclude='CMakeFiles/' \
  --exclude='CMakeCache.txt' \
  --exclude='*.o' \
  --exclude='*.snap' \
  --exclude='*.zip' \
  --exclude='*.log' \
  "$SOURCE_DIR/" "$STAGE_DIR/"

say "2/6" "Checking known-good Fractionstutor Snapcraft configuration"
YAML="$STAGE_DIR/snapcraft.yaml"
[[ -f "$YAML" ]] || { echo "Missing snapcraft.yaml" >&2; exit 1; }
grep -Eq '^base:[[:space:]]*core24[[:space:]]*$' "$YAML" || { echo "snapcraft.yaml does not declare base: core24" >&2; exit 1; }
grep -q 'kde-neon-6' "$YAML" || { echo "snapcraft.yaml does not contain the kde-neon-6 extension needed by this Qt 6 build." >&2; exit 1; }
grep -q -- '-DCMAKE_INSTALL_PREFIX=/usr' "$YAML" || { echo "snapcraft.yaml is missing -DCMAKE_INSTALL_PREFIX=/usr" >&2; exit 1; }

say "3/6" "Building with Snapcraft + Multipass"
cd "$STAGE_DIR"
(
    while true; do
        sleep 15
        now="$(date +%s)"
        elapsed=$((now - START_TS))
        printf '[3/6] build running... elapsed %02d:%02d (Ctrl+C cancels)\n' "$((elapsed/60))" "$((elapsed%60))" >&2
    done
) &
TIMER_PID=$!
(
  export SNAPCRAFT_BUILD_ENVIRONMENT=multipass
  exec snapcraft pack
) &
BUILD_PID=$!
set +e
wait "$BUILD_PID"
BUILD_STATUS=$?
set -e
BUILD_PID=""
stop_timer
if [[ "$BUILD_STATUS" -ne 0 ]]; then
    echo "Snapcraft failed with exit code $BUILD_STATUS." >&2
    exit "$BUILD_STATUS"
fi

say "4/6" "Locating and inspecting generated snap"
mapfile -t SNAPS < <(find "$STAGE_DIR" -maxdepth 1 -type f -name '*.snap' -printf '%T@ %p\n' | sort -nr | cut -d' ' -f2-)
(( ${#SNAPS[@]} > 0 )) || { echo "No .snap file was produced." >&2; exit 1; }
NEW_SNAP="${SNAPS[0]}"
ls -lh "$NEW_SNAP"
CONTENTS="$(unsquashfs -l "$NEW_SNAP")"
grep -q 'squashfs-root/usr/bin/fractionstutor' <<<"$CONTENTS" || { echo "Verification failed: usr/bin/fractionstutor is missing." >&2; exit 1; }
grep -q 'squashfs-root/meta/snap.yaml' <<<"$CONTENTS" || { echo "Verification failed: meta/snap.yaml is missing." >&2; exit 1; }
printf 'Verified required payload: usr/bin/fractionstutor and meta/snap.yaml\n'

say "5/6" "Copying snap back to project"
cp -f "$NEW_SNAP" "$OUTPUT_DIR/"
FINAL_SNAP="$OUTPUT_DIR/$(basename "$NEW_SNAP")"
printf 'Saved: %s\n' "$FINAL_SNAP"

say "6/6" "Final report"
END_TS="$(date +%s)"
ELAPSED=$((END_TS - START_TS))
printf 'SUCCESS\n'
printf 'Snap: %s\n' "$FINAL_SNAP"
printf 'Elapsed: %02d:%02d\n' "$((ELAPSED/60))" "$((ELAPSED%60))"
printf 'Next local test (requires your authorization): sudo snap install "%s" --dangerous\n' "$FINAL_SNAP"
printf 'Then run: snap run fractionstutor\n'
