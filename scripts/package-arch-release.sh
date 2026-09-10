#!/usr/bin/env bash
set -euo pipefail

version=${1:?"Usage: $0 <version>"}
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_dir=${BUILD_DIR:-"$root/build"}
name="kde-scroll-fix-${version}-arch-x86_64"
dist_dir="$root/dist"
stage_dir="$dist_dir/$name"
archive="$dist_dir/$name.tar.gz"

for file in scrollfix.so scroll-fix-settings; do
    [[ -x "$build_dir/$file" || -f "$build_dir/$file" ]] || {
        echo "Missing build artifact: $build_dir/$file" >&2
        exit 1
    }
done

rm -rf "$stage_dir"
mkdir -p "$stage_dir"
install -m 755 "$build_dir/scrollfix.so" "$stage_dir/scrollfix.so"
install -m 755 "$build_dir/scroll-fix-settings" "$stage_dir/scroll-fix-settings"
install -m 644 "$root/scroll-fix-settings.desktop" "$stage_dir/scroll-fix-settings.desktop"

cat >"$stage_dir/install.sh" <<'INSTALLER'
#!/usr/bin/env bash
set -euo pipefail

fail() {
    echo "error: $*" >&2
    exit 1
}

[[ $(uname -m) == x86_64 ]] || fail "This release supports x86_64 only."
command -v kwin_wayland >/dev/null || fail "KWin is not installed."

kwin_version=$(kwin_wayland --version | awk '{print $2}')
[[ $kwin_version == 6.7.* ]] || fail "This release requires KWin 6.7.x; found ${kwin_version:-unknown}."

plugin_dirs=(/usr/lib/qt6/plugins /usr/lib64/qt6/plugins /usr/lib/qt/plugins)
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

root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
sudo install -Dm 755 "$root/scrollfix.so" "$plugin_dir/kwin/plugins/scrollfix.so"
install -Dm 755 "$root/scroll-fix-settings" "$HOME/.local/bin/scroll-fix-settings"
install -Dm 644 "$root/scroll-fix-settings.desktop" \
    "$HOME/.local/share/applications/scroll-fix-settings.desktop"

echo "Installed KDE Scroll Fix for KWin $kwin_version in $plugin_dir."
echo "Open Touchpad Scroll Settings, or run scroll-fix-settings."
INSTALLER
chmod 755 "$stage_dir/install.sh"

cat >"$dist_dir/install-arch.sh" <<INSTALLER
#!/usr/bin/env bash
set -euo pipefail

version="$version"
name="$name"
tmp_dir=\$(mktemp -d)
trap 'rm -rf "\$tmp_dir"' EXIT

curl --fail --location --proto '=https' --tlsv1.2 \\
    -o "\$tmp_dir/\$name.tar.gz" \\
    "https://github.com/jaasonw/kde-scroll-fix/releases/download/\$version/\$name.tar.gz"
tar -xzf "\$tmp_dir/\$name.tar.gz" -C "\$tmp_dir"
"\$tmp_dir/\$name/install.sh"
INSTALLER
chmod 755 "$dist_dir/install-arch.sh"

tar -C "$dist_dir" -czf "$archive" "$name"
printf 'Created:\n%s\n%s\n' "$archive" "$dist_dir/install-arch.sh"
