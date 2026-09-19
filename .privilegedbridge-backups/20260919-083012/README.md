# PrivilegedBridge 0.1

PrivilegedBridge is a Qt/C++ skeleton that lets local AI agents request **named, allowlisted actions** without giving them an unrestricted shell or network service.

This first version intentionally has no TCP/UDP listener and uses only local process invocation. Vibe or another local agent can inspect available actions with:

```bash
./PrivilegedBridge --list
./PrivilegedBridge --status
```

and request an allowlisted action with:

```bash
./PrivilegedBridge --run check_lxd_installed
./PrivilegedBridge --run install_lxd
```

Privileged actions are wrapped with `pkexec`; authorization remains with the logged-in user/polkit. The program does not store a sudo password.

Built-in actions:

- `check_lxd_installed`
- `install_lxd`
- `initialize_lxd`
- `add_user_to_lxd_group`
- `check_lxd_ready`

To add more privileged actions safely, extend `loadBuiltinActions()` in `mainwindow.cpp` and rebuild. This avoids exposing an arbitrary `sudo <anything>` interface to an AI agent.

Provenance identifier: `TG239670`.
