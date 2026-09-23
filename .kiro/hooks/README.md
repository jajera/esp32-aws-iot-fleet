# Hooks

Kiro v1 hooks ([reference](https://kiro.dev/docs/hooks/)). Each file declares
`version: "v1"` and a `hooks` array.

| File | Trigger | Blocks | Purpose |
|------|---------|--------|---------|
| `guard-aws-mutations.json` | `PreToolUse` | yes | Refuses mutating AWS CLI / IaC from shell tools |

## guard-aws-mutations

```bash
export FLEET_ALLOW_AWS=1
```

Quick test:

```bash
echo '{"command":"aws iot create-thing --thing-name x"}' | python3 .kiro/hooks/guard-aws-mutations.py; echo "exit=$?"
echo '{"command":"aws iot list-things"}' | python3 .kiro/hooks/guard-aws-mutations.py; echo "exit=$?"
```

See `.kiro/steering/lab-safety.md`.
