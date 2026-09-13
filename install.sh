#!/usr/bin/env bash
set -euo pipefail

fail() {
    echo "error: $*" >&2
    exit 1
}

[[ $(uname -m) == x86_64 ]] || fail "This release supports x86_64 only."
command -v kwin_wayland >/dev/null || fail "KWin is not installed."

kwin_version=$(kwin_wayland --version | awk '{print $2}')
[[ $kwin_version == 6.7.* ]] || fail "This prebuilt release requires KWin 6.7.x; found ${kwin_version:-unknown}.
Build from source instead: https://github.com/jaasonw/kde-scroll-fix#install-build-from-source"

root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
artifact_dir=$root
[[ -f "$artifact_dir/scrollfix.so" ]] || artifact_dir="$root/build"
for file in scrollfix.so scroll-fix-settings; do
    [[ -f "$artifact_dir/$file" ]] || fail "Missing build artifact: $artifact_dir/$file"
done

# Debian/Ubuntu use a multiarch libdir; Arch uses /usr/lib; Fedora/openSUSE use /usr/lib64.
plugin_dirs=(
    "/usr/lib/$(uname -m)-linux-gnu/qt6/plugins"
    /usr/lib/qt6/plugins
    /usr/lib64/qt6/plugins
    /usr/lib/qt/plugins
)
if command -v qmake6 >/dev/null; then
    plugin_dirs=("$(qmake6 -query QT_INSTALL_PLUGINS)" "${plugin_dirs[@]}")
elif command -v qtpaths6 >/dev/null; then
    plugin_dirs=("$(qtpaths6 --plugin-dir)" "${plugin_dirs[@]}")
elif command -v qtpaths >/dev/null; then
    plugin_dirs=("$(qtpaths --plugin-dir)" "${plugin_dirs[@]}")
fi

plugin_dir=
for candidate in "${plugin_dirs[@]}"; do
    if [[ -d "$candidate/kwin/plugins" ]]; then
        plugin_dir=$candidate
        break
    fi
done
[[ -n $plugin_dir ]] || fail "Cannot find KWin's Qt plugin directory. Install qmake6 or qtpaths, then retry."

sudo install -Dm 755 "$artifact_dir/scrollfix.so" "$plugin_dir/kwin/plugins/scrollfix.so"
install -Dm 755 "$artifact_dir/scroll-fix-settings" "$HOME/.local/bin/scroll-fix-settings"
install -Dm 644 "$root/scroll-fix-settings.desktop" \
    "$HOME/.local/share/applications/scroll-fix-settings.desktop"

echo "Installed KDE Scroll Fix for KWin $kwin_version in $plugin_dir."
echo "Open Touchpad Scroll Settings, or run scroll-fix-settings."
