#!/usr/bin/env bash
# Field Ambience PCB — Auto-ERC-Lauf via kicad-cli
#
# Usage:
#   cd field-ambience-current
#   bash scripts/run_erc.sh
#
# Voraussetzung: KiCad 9.0+ installiert. Findet kicad-cli automatisch
# auf macOS/Linux/Windows-WSL.

set -e

# Find kicad-cli
KICAD_CLI=""
for candidate in \
    "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli" \
    "/usr/bin/kicad-cli" \
    "/usr/local/bin/kicad-cli" \
    "$(command -v kicad-cli || true)"; do
    if [[ -x "$candidate" ]]; then
        KICAD_CLI="$candidate"
        break
    fi
done

if [[ -z "$KICAD_CLI" ]]; then
    echo "ERROR: kicad-cli nicht gefunden."
    echo ""
    echo "Bitte KiCad 9.0+ installieren:"
    echo "  macOS:   https://www.kicad.org/download/macos/"
    echo "  Windows: https://www.kicad.org/download/windows/"
    echo "  Linux:   sudo apt install kicad"
    echo ""
    echo "Auf macOS ist kicad-cli unter:"
    echo "  /Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
    exit 1
fi

echo "✓ Gefunden: $KICAD_CLI"
"$KICAD_CLI" version 2>&1 | head -3

# Repo-Root finden (Script läuft entweder aus field-ambience-current/ oder scripts/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

KICAD_PROJECT_DIR="$PROJECT_ROOT/kicad"
ROOT_SCH="$KICAD_PROJECT_DIR/field_ambience.kicad_sch"
REPORTS_DIR="$PROJECT_ROOT/reports"
DATE="$(date +%Y-%m-%d)"
REPORT_TXT="$REPORTS_DIR/ERC_${DATE}.txt"
REPORT_MD="$REPORTS_DIR/ERC_${DATE}.md"

if [[ ! -f "$ROOT_SCH" ]]; then
    echo "ERROR: Schematic nicht gefunden: $ROOT_SCH"
    echo "Stelle sicher dass du im richtigen Repo bist."
    exit 1
fi

mkdir -p "$REPORTS_DIR"

echo ""
echo "==> Führe ERC aus auf $ROOT_SCH ..."
"$KICAD_CLI" sch erc \
    --severity-all \
    --output "$REPORT_TXT" \
    "$ROOT_SCH" || true  # ERC returns non-zero if findings exist, das ist OK

if [[ ! -f "$REPORT_TXT" ]]; then
    echo "ERROR: ERC-Report wurde nicht generiert."
    exit 1
fi

# Parse Errors/Warnings für Summary
ERRORS=$(grep -ci "^.* error\|severity: error" "$REPORT_TXT" 2>/dev/null || echo 0)
WARNINGS=$(grep -ci "^.* warning\|severity: warning" "$REPORT_TXT" 2>/dev/null || echo 0)
# Alternative parsing
TOTAL=$(grep -c "^\(\[\| \+\)" "$REPORT_TXT" 2>/dev/null || echo "?")

# Markdown-Wrapper schreiben
GIT_SHA="$(cd "$PROJECT_ROOT/.." && git rev-parse --short HEAD 2>/dev/null || echo unknown)"
KICAD_VER="$("$KICAD_CLI" version 2>&1 | head -1 | tr -d '\n')"

cat > "$REPORT_MD" <<EOF
# KiCad ERC Report — Field Ambience PCB

**Date**: $DATE
**KiCad Version**: $KICAD_VER
**Git Commit**: $GIT_SHA
**Schematic**: \`kicad/field_ambience.kicad_sch\`

## Summary

- Errors: $ERRORS
- Warnings: $WARNINGS

## Full Report

\`\`\`
$(cat "$REPORT_TXT")
\`\`\`

## Acceptance Status

- [ ] All errors reviewed
- [ ] Warnings triaged (accepted with reason OR fixed)
- [ ] Cleared for PCB-Layout phase

EOF

echo ""
echo "==> ERC-Lauf fertig."
echo "    Errors:   $ERRORS"
echo "    Warnings: $WARNINGS"
echo ""
echo "    TXT-Report: $REPORT_TXT"
echo "    MD-Report:  $REPORT_MD"
echo ""

if [[ "$ERRORS" -gt 0 ]]; then
    echo "⚠️  ERC hat ERRORS gefunden — siehe Report. NICHT für PCB-Layout freigegeben."
    exit 0  # don't fail the script — let user inspect
else
    echo "✓  ERC clean. Bereit für nächsten Schritt (Footprint-Check + PCB-Layout)."
fi
