#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
exec /bin/bash "$ROOT/app/package.command" "$@"
