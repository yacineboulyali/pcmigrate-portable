#!/bin/sh
# Creates the downloadable files in dist/ :
#   PCMigrate.exe, PCMigrate-Setup-<ver>.exe (installer), PCMigrate-Portable-<ver>.zip
#   needs: nsis (makensis) and zip
set -e
cd "$(dirname "$0")"
VER=$(cat VERSION | tr -d '[:space:]')
[ -f build/PCMigrate.exe ] || ./build.sh
rm -rf dist && mkdir -p dist/portable/PCMigrate
cp build/PCMigrate.exe dist/
# portable package
cp build/PCMigrate.exe launcher/Run-PCMigrate.bat launcher/Copy-To-USB.bat dist/portable/PCMigrate/
cp docs/README_AR.md dist/portable/PCMigrate/README_AR.md
cp LICENSE dist/portable/PCMigrate/LICENSE.txt
(cd dist/portable && zip -qr "../PCMigrate-Portable-$VER.zip" PCMigrate)
rm -rf dist/portable
# installer
makensis -V2 -DVERSION="$VER" installer/PCMigrate.nsi
echo "OK -> dist/"
ls -la dist
