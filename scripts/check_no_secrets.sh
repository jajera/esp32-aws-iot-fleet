#!/usr/bin/env bash
# Fail if staged or tracked files look like secrets.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

# Split markers so this script does not match its own source when scanned.
pem_rsa="BEGIN RSA PRIVATE KEY"
pem_ec="BEGIN EC PRIVATE KEY"
pem_any="BEGIN PRIVATE KEY"

path_patterns=(
  '\.pem$'
  '\.key$'
  '\.p12$'
  '\.pfx$'
  'secrets\.ini$'
  'claim-key'
  'device-key'
)

# Tracked files that should never exist
tracked="$(git ls-files)"
while IFS= read -r f; do
  [[ -z "$f" ]] && continue
  case "$f" in
    secrets/README.md|certs/README.md|scripts/check_no_secrets.sh) continue ;;
  esac
  for pat in "${path_patterns[@]}"; do
    if echo "$f" | grep -Eq "$pat"; then
      echo "Refusing secret-like tracked file: $f" >&2
      exit 1
    fi
  done
done <<< "$tracked"

# Staged content scan for private key markers
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  staged="$(git diff --cached --name-only 2>/dev/null || true)"
  if [[ -n "${staged:-}" ]]; then
    while IFS= read -r f; do
      [[ -z "$f" ]] && continue
      case "$f" in
        scripts/check_no_secrets.sh) continue ;;
      esac
      if git show ":$f" 2>/dev/null | grep -Eq "${pem_rsa}|${pem_ec}|${pem_any}"; then
        echo "Refusing staged private key material in: $f" >&2
        exit 1
      fi
    done <<< "$staged"
  fi
fi

echo "Secret check passed"
