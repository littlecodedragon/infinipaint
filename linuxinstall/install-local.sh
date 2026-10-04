#!/usr/bin/env bash
# Install a built InfiniPaint binary from this repo into ~/.local
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${1:-$ROOT/build/Release/infinipaint}"
PREFIX="${HOME}/.local"
APPDIR="${PREFIX}/share/infinipaint"

[[ -x "$BIN" ]] || { echo "missing binary: $BIN" >&2; exit 1; }

mkdir -p "$APPDIR" "${PREFIX}/bin" "${PREFIX}/share/applications"
install -m755 "$BIN" "${APPDIR}/infinipaint"
rm -rf "${APPDIR}/data"
cp -a "${ROOT}/assets/data" "${APPDIR}/data"
ln -sfn "${APPDIR}/infinipaint" "${PREFIX}/bin/infinipaint"

cat > "${PREFIX}/share/applications/com.infinipaint.infinipaint-fork.desktop" <<EOF
[Desktop Entry]
Name=InfiniPaint (fork)
Comment=Infinite collaborative canvas (copilot fork)
Exec=${PREFIX}/bin/infinipaint
Icon=com.infinipaint.infinipaint
Terminal=false
Type=Application
Categories=Graphics;
EOF

echo "Installed: ${PREFIX}/bin/infinipaint"
# GUI binary may abort without a working GL context; do not fail install.
"${PREFIX}/bin/infinipaint" --version >/dev/null 2>&1 || true
