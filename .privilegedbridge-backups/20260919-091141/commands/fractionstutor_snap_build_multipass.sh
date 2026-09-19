#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR="${1:-/home/we6jbo/Projects/fractionstutor}"
STAGE_ROOT="${XDG_CACHE_HOME:-$HOME/.cache}/privileged-bridge/snap-staging"
STAGE_DIR="$STAGE_ROOT/fractionstutor"
OUTPUT_DIR="$SOURCE_DIR"
BUILD_PID=""

say() { printf '\n[%s] %s\n' "$1" "$2"; }

cleanup_on_cancel() {
    printf '\nCancel requested. Stopping Snapcraft...\n' >&2
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
if ! command -v rsync >/dev/null 2>&1; then
    echo "rsync is required. Install it with: sudo pacman -S rsync" >&2
    exit 1
fi
if ! command -v snapcraft >/dev/null 2>&1; then
    echo "snapcraft was not found in PATH." >&2
    exit 1
fi

say "1/5" "Preparing clean Snapcraft staging tree"
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

say "2/5" "Checking Snapcraft project files"
if [[ ! -f "$STAGE_DIR/snapcraft.yaml" && ! -f "$STAGE_DIR/snap/snapcraft.yaml" ]]; then
    echo "No snapcraft.yaml was found in the clean staging tree." >&2
    exit 1
fi

say "3/5" "Starting Multipass-backed Snapcraft build"
echo "Press Ctrl+C at any time to request cancellation."
cd "$STAGE_DIR"
(
  export SNAPCRAFT_BUILD_ENVIRONMENT=multipass
  exec snapcraft pack
) &
BUILD_PID=$!
wait "$BUILD_PID"
BUILD_STATUS=$?
BUILD_PID=""
if [[ "$BUILD_STATUS" -ne 0 ]]; then
    echo "Snapcraft failed with exit code $BUILD_STATUS." >&2
    exit "$BUILD_STATUS"
fi

say "4/5" "Locating generated snap"
mapfile -t SNAPS < <(find "$STAGE_DIR" -maxdepth 1 -type f -name '*.snap' -printf '%T@ %p\n' | sort -nr | cut -d' ' -f2-)
if (( ${#SNAPS[@]} == 0 )); then
    echo "Snapcraft finished but no .snap file was found in $STAGE_DIR" >&2
    exit 1
fi
NEW_SNAP="${SNAPS[0]}"
echo "Built: $NEW_SNAP"
ls -lh "$NEW_SNAP"

say "5/5" "Copying snap back to project"
cp -f "$NEW_SNAP" "$OUTPUT_DIR/"
FINAL_SNAP="$OUTPUT_DIR/$(basename "$NEW_SNAP")"
echo "Saved: $FINAL_SNAP"
echo
echo "Build complete."
echo "Inspect with: unsquashfs -l \"$FINAL_SNAP\" | less"
echo "Test install with: sudo snap install \"$FINAL_SNAP\" --dangerous"
