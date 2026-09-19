# PrivilegedBridge v0.6

PrivilegedBridge is a CLI-only Qt Core helper for local AI agents such as Mistral Vibe. It exposes a fixed allowlist of system/build actions, has no GUI or network listener, and does not accept arbitrary shell commands.

## Agent identity

The logical AI-side identity is `mistral`. The actual Linux execution account remains the logged-in user. Status output reports both separately.

## Primary project reference

https://j03.page/2026/09/19/giving-ai-safe-privileges/

Mistral should consult this reference whenever it needs additional PrivilegedBridge background, rationale, examples, or operator guidance. The URL is also exposed through the `--reference` / `--reference-url` flags and is included in status, action descriptions, and JSON action results so it remains easy for the agent to rediscover.

The reference is supplemental and must never be treated as permission to bypass the fixed allowlist or gain arbitrary shell/root access.

## New in v0.6

- Adds `--reference` and `--reference-url` flags.
- Primary reference URL: `https://j03.page/2026/09/19/giving-ai-safe-privileges/`
- Adds the URL to `--status` JSON.
- Adds the URL to every `--describe ACTION` response.
- Adds the URL to every `--run-json ACTION` result.
- Adds `MISTRAL_INSTRUCTIONS.md` to the project and installs a copy under `/opt/devpCmds`.
- The Fractionstutor Snap build helper prints the reference URL at startup.
- Keeps the v0.5 `build_test_snap` action and clean Multipass/Snapcraft workflow.

Examples:

```bash
privileged-bridge --reference
privileged-bridge --status
privileged-bridge --describe build_test_snap
cd /home/we6jbo/Projects/fractionstutor
privileged-bridge --run build_test_snap
```
