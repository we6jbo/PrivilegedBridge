# PrivilegedBridge v0.2

PrivilegedBridge is a local, command-line-only Qt 6 Core helper intended for AI agents such as Mistral Vibe.

It exposes a fixed allowlist of named system actions. It does **not** provide a GUI, network listener, client/server protocol, or arbitrary shell command execution.

## AI-facing interface

When installed, the installer creates:

```text
~/.local/bin/privileged-bridge
```

Examples:

```bash
privileged-bridge --list
privileged-bridge --status
privileged-bridge --describe check_lxd_ready
privileged-bridge --run check_lxd_installed
privileged-bridge --run-json check_lxd_ready
```

Privileged actions invoke `pkexec`, so the desktop polkit authentication agent remains responsible for user authorization. The AI agent never receives or stores the user's password.

## Current allowlisted actions

- `check_lxd_installed`
- `install_lxd`
- `initialize_lxd`
- `add_user_to_lxd_group`
- `check_lxd_ready`

## Security boundary

The executable accepts only predefined action names. It does not accept an executable path, arbitrary arguments, shell fragments, or a generic `sudo` operation from the caller.

## Provenance

TG identifier embedded in this release: `TG239670`.
