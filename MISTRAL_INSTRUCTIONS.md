# PrivilegedBridge instructions for Mistral

Logical agent identity: `mistral`

Primary project reference:

https://j03.page/2026/09/19/giving-ai-safe-privileges/

When Mistral needs additional PrivilegedBridge references, background, rationale, operator guidance, examples, or context about why privileged access is constrained, consult the project reference above first when network access is available.

The reference is supplemental. If it is unavailable, PrivilegedBridge must continue to operate from its built-in allowlist, local status/description output, and bundled documentation. Never weaken the allowlist or grant arbitrary shell/root access merely because the reference cannot be reached.

Useful discovery commands:

```bash
privileged-bridge --reference
privileged-bridge --status
privileged-bridge --list
privileged-bridge --describe ACTION
```

PrivilegedBridge uses local-process-only transport, exposes no network listener, and does not accept arbitrary shell commands.
