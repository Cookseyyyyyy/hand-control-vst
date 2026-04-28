#!/usr/bin/env bash
#
# Builds a signed/notarized macOS installer that drops the VST3 and AU bundles
# into the system plugin folders. Requires:
#   - Xcode command line tools
#   - A Developer ID Application cert for codesigning the plugins
#   - A Developer ID Installer cert for productsign
#   - `xcrun notarytool` credentials stored under a keychain profile (see below)
#
# Usage: ./build_pkg.sh VERSION [BUILD_DIR]

set -euo pipefail

VERSION="${1:-0.1.0}"
BUILD_DIR="${2:-$(cd "$(dirname "$0")/../.." && pwd)/build}"
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="$HERE/out"
mkdir -p "$OUT"

DEV_ID_APP="${DEV_ID_APP:-Developer ID Application: YOUR NAME (TEAMID)}"
DEV_ID_INSTALLER="${DEV_ID_INSTALLER:-Developer ID Installer: YOUR NAME (TEAMID)}"
NOTARY_PROFILE="${NOTARY_PROFILE:-handcontrol-notary}"

VST3="$BUILD_DIR/plugin/HandControlPlugin_artefacts/Release/VST3/Hand Control.vst3"
AU="$BUILD_DIR/plugin/HandControlPlugin_artefacts/Release/AU/Hand Control.component"

if [ ! -d "$VST3" ]; then echo "Missing $VST3"; exit 1; fi
if [ ! -d "$AU"   ]; then echo "Missing $AU";   exit 1; fi

# -- codesign both bundles --------------------------------------------------
codesign --force --deep --options runtime \
    --sign "$DEV_ID_APP" \
    "$VST3"
codesign --force --deep --options runtime \
    --sign "$DEV_ID_APP" \
    "$AU"

# -- stage payload ---------------------------------------------------------
STAGE="$OUT/stage"
rm -rf "$STAGE"
mkdir -p "$STAGE/Library/Audio/Plug-Ins/VST3"
mkdir -p "$STAGE/Library/Audio/Plug-Ins/Components"
cp -R "$VST3" "$STAGE/Library/Audio/Plug-Ins/VST3/"
cp -R "$AU"   "$STAGE/Library/Audio/Plug-Ins/Components/"

# -- build component pkg ----------------------------------------------------
pkgbuild \
    --root "$STAGE" \
    --identifier com.handcontrol.HandControl \
    --version "$VERSION" \
    --install-location "/" \
    --sign "$DEV_ID_INSTALLER" \
    "$OUT/HandControl-component.pkg"

# -- build product pkg ------------------------------------------------------
DIST="$OUT/distribution.xml"
cat > "$DIST" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>Hand Control</title>
    <organization>com.handcontrol</organization>
    <pkg-ref id="com.handcontrol.HandControl"/>
    <choices-outline>
        <line choice="default">
            <line choice="com.handcontrol.HandControl"/>
        </line>
    </choices-outline>
    <choice id="default"/>
    <choice id="com.handcontrol.HandControl" visible="false">
        <pkg-ref id="com.handcontrol.HandControl"/>
    </choice>
    <pkg-ref id="com.handcontrol.HandControl" version="$VERSION" onConclusion="none">HandControl-component.pkg</pkg-ref>
</installer-gui-script>
EOF

productbuild \
    --distribution "$DIST" \
    --package-path "$OUT" \
    --sign "$DEV_ID_INSTALLER" \
    "$OUT/HandControl-$VERSION.pkg"

# -- notarize + staple -----------------------------------------------------
xcrun notarytool submit "$OUT/HandControl-$VERSION.pkg" \
    --keychain-profile "$NOTARY_PROFILE" \
    --wait

xcrun stapler staple "$OUT/HandControl-$VERSION.pkg"

echo "Built: $OUT/HandControl-$VERSION.pkg"
