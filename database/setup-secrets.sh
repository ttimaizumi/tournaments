#!/usr/bin/env bash
# Run with Bash; encrypted credentials are the only generated repository file.
set +x
set -euo pipefail
umask 077

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root"
encrypted=database/credentials.enc.env
export SOPS_AGE_KEY_FILE=${SOPS_AGE_KEY_FILE:-${XDG_CONFIG_HOME:-$HOME/.config}/sops/age/keys.txt}

need() {
  command -v "$1" >/dev/null || { echo "Required command not found: $1" >&2; exit 1; }
}

usage() {
  echo "Usage: bash database/setup-secrets.sh {init|apply}" >&2
  exit 1
}

[[ $# == 1 ]] || usage
need sops
case "$1" in
  init)
    need age-keygen
    need python3
    [[ ! -e "$encrypted" ]] || {
      echo "$encrypted already exists; refusing to replace credentials." >&2
      exit 1
    }
    if [[ ! -e "$SOPS_AGE_KEY_FILE" ]]; then
      mkdir -p -- "$(dirname -- "$SOPS_AGE_KEY_FILE")"
      age-keygen -o "$SOPS_AGE_KEY_FILE"
    fi
    recipient=$(age-keygen -y "$SOPS_AGE_KEY_FILE")
    # This temporary file contains ciphertext only. A failed encryption is never
    # published under the final filename. ln refuses to replace an existing file.
    temporary=$(mktemp database/.credentials.enc.XXXXXX)
    trap 'rm -f -- "$temporary"' EXIT
    python3 - <<'PY' | sops encrypt --input-type dotenv --output-type dotenv --age "$recipient" /dev/stdin > "$temporary"
import pathlib
import secrets
import sys

keys = ('POSTGRES_PASSWORD', 'TOURNAMENT_ADMIN_PASSWORD', 'TOURNAMENT_SVC_PASSWORD')
existing = pathlib.Path('.env')
if existing.exists():
    values = {}
    for line in existing.read_text().splitlines():
        if not line.strip() or line.lstrip().startswith('#'):
            continue
        key, separator, value = line.partition('=')
        if separator and key in keys:
            # The previous setup generated unquoted hex values. Refuse ambiguous
            # dotenv syntax instead of silently changing an existing password.
            if not value or any(c in value for c in " \t\r\n\"'`$#\\"):
                sys.exit('Unsupported .env password format; migrate manually with SOPS.')
            if key in values:
                sys.exit('Duplicate credential key in .env; resolve before migration.')
            values[key] = value
    if set(values) != set(keys):
        sys.exit('Existing .env is missing database credentials; refusing to generate replacements.')
else:
    values = {key: secrets.token_hex(32) for key in keys}
for key in keys:
    print(f'{key}={values[key]}')
PY
    ln -- "$temporary" "$encrypted"
    echo "Created $encrypted. Back up your age private key outside this repository."
    ;;
  apply)
    need kubectl
    [[ -f "$encrypted" ]] || { echo "Run init first or restore $encrypted from Git." >&2; exit 1; }
    # Check decryption before making any cluster changes. Do not print plaintext.
    sops decrypt --input-type dotenv --output-type dotenv "$encrypted" >/dev/null
    kubectl create namespace tournaments --dry-run=client -o yaml | kubectl apply --server-side -f -
    sops decrypt --input-type dotenv --output-type dotenv "$encrypted" |
      kubectl -n tournaments create secret generic tournament-db-credentials \
        --from-env-file=/dev/stdin --dry-run=client -o yaml |
      kubectl apply --server-side --field-manager=tournament-secrets -f -
    ;;
  *) usage ;;
esac
