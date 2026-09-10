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

install -m 755 "$root/install.sh" "$stage_dir/install.sh"

cat >"$dist_dir/install.sh" <<INSTALLER
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
chmod 755 "$dist_dir/install.sh"

tar -C "$dist_dir" -czf "$archive" "$name"
printf 'Created:\n%s\n%s\n' "$archive" "$dist_dir/install.sh"
