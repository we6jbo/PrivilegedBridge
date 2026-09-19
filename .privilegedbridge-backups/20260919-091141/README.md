# PrivilegedBridge v0.3

PrivilegedBridge is a CLI-only Qt Core helper intended for local AI agents such as Mistral Vibe. It exposes a fixed allowlist of system and build actions without providing arbitrary shell execution, a GUI, or a network listener.

## New in v0.3

- Multipass fallback checks and installation.
- Snapcraft provider diagnostics.
- Ability to force Snapcraft builds through Multipass with `SNAPCRAFT_BUILD_ENVIRONMENT=multipass`.
- LXD activation log inspection.
- snapd/AppArmor/confinement diagnostics.
- `/dev/kvm` readiness check.
- Snapcraft installation check and allowlisted installation.
- Snap command resolver also checks `/snap/bin` and `/var/lib/snapd/snap/bin` when the current PATH has not refreshed.

## Examples

```bash
privileged-bridge --list
privileged-bridge --run-json check_snapcraft_provider
privileged-bridge --run check_lxd_activation_log
privileged-bridge --run install_multipass
privileged-bridge --run check_multipass_ready
privileged-bridge --run build_snap_with_multipass
```

Privileged actions go through `pkexec`. No password is stored or passed to the AI.

## v0.4 command staging helper

`commands/fractionstutor_snap_build_multipass.sh` prepares a clean copy of the Fractionstutor project that excludes generated Flatpak, Git, CMake and previous Snap artifacts before invoking `snapcraft pack` with Multipass. It prints five build stages and Ctrl+C requests cancellation of the active Snapcraft process.

The installer also places this helper at `/opt/devpCmds/fractionstutor_snap_build_multipass.sh` when sudo authorization is available. PrivilegedBridge itself remains an allowlisted local-process CLI and does not open a network listener.
