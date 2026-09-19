# PrivilegedBridge v0.5

PrivilegedBridge is a CLI-only Qt Core helper for local AI agents such as Mistral Vibe. It exposes a fixed allowlist of system/build actions, has no GUI or network listener, and does not accept arbitrary shell commands.

## Agent identity

The logical AI-side identity is `mistral`. The actual Linux execution account remains the logged-in user. Status output reports both separately.

## New in v0.5

- Adds `build_test_snap` to the allowlist.
- `build_test_snap` invokes `/opt/devpCmds/fractionstutor_snap_build_multipass.sh`.
- Uses the working clean-staging recipe for Fractionstutor.
- Requires the known-good `core24` + `kde-neon-6` Snapcraft configuration before launching the slow VM build.
- Uses `snapcraft pack`, not deprecated bare `snapcraft`.
- Prints stage progress and elapsed-time heartbeat messages while Multipass/Snapcraft runs.
- Ctrl+C requests cancellation of the active build.
- Verifies that the generated snap contains `usr/bin/fractionstutor` and `meta/snap.yaml`.
- Copies the completed snap back to `/home/we6jbo/Projects/fractionstutor`.

Examples:

```bash
privileged-bridge --list
privileged-bridge --status
privileged-bridge --describe build_test_snap
cd /home/we6jbo/Projects/fractionstutor
privileged-bridge --run build_test_snap
```
