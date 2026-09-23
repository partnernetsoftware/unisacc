#!/usr/bin/env bash
# Rebuild all GitHub Pages UXE games (asteroid + drone).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT/ujs"
perl -e 'alarm 180; exec @ARGV' npm run ship:engine
perl -e 'alarm 120; exec @ARGV' npm run ship:drone
echo "OK_SHIP_PAGES"
