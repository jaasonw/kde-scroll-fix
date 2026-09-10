# kde-scroll-fix

## Fix touchpad scrolling in Chromium-based apps

Reduces touchpad scroll speed in Chromium-based applications on KDE Plasma with Wayland.

Chromium, Brave, Chrome, Discord, VS Code, and other Electron applications have had a known issue for YEARS that
caused trackpads to scroll faster than the native app setting. I have no interest in waiting for Chromium/Linux/Wayland/etc devs to keep fighting turf wars about whos problem it is to fix what or whiney opinioned users who will tell me their wife's boyfriend says turbo scrolling is good and normal or whatever, meanwhile ChromeOS literally just has trackpad scrolling solved so I had GPT cook up a fix for my own specific setup.

From an average user's POV, scrolling on Chromium apps should literally just work especially on a laptop, "use a mouse 4head" or "learn to keyboard motions only 4head" is NOT an acceptable solution and the people who suggest this can suck my nuts

This is an experimental KWin plugin. Select applications by clicking their windows, then set one scroll-speed factor for every selected application. The plugin does not affect unselected applications, mouse wheels, gestures, or pinch-to-zoom events.

> **Warning:** KDE Scroll Fix runs inside the compositor. Read [Risks and rollback](#risks-and-rollback) before enabling it at startup.

## Install (Pre built binary)

The prebuilt binary was built on x86_64 Arch Linux with KWin 6.7.x. It may work for other distros but I have only tested it on an Arch system

```sh
curl -fsSL https://github.com/jaasonw/kde-scroll-fix/releases/download/v0.1.0-alpha.1/install-arch.sh | bash
```

## Before you begin

You need the following:

- KDE Plasma on **Wayland**. X11 is not supported.
- **KWin 6.3 or later**. The plugin uses `InputEventFilter::pointerAxis(PointerAxisEvent *)`.
- KWin development headers, CMake 3.25 or later, a C++20 compiler, Qt 6, and KF6.

### Install dependencies

Install the direct build dependencies for your distribution. The KWin development package pulls the required Qt 6 and KDE Frameworks 6 development packages.

#### Arch Linux

```sh
sudo pacman -S --needed base-devel cmake extra-cmake-modules kwin
```

#### Fedora

```sh
sudo dnf install gcc-c++ make cmake extra-cmake-modules kwin-devel
```

#### Debian 13+

```sh
sudo apt install build-essential cmake extra-cmake-modules kwin-dev
```

#### Ubuntu 25.04+

```sh
sudo apt install build-essential cmake extra-cmake-modules kwin-dev
```

Ubuntu 24.04 LTS does not provide the required KWin 6.3 and KDE Frameworks 6 stack.

## Install KDE Scroll Fix

### Build and test the project

Run the following commands:

```sh
cmake -S . -B build \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DKDE_INSTALL_PLUGINDIR=lib/qt6/plugins
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

The `lib/qt6/plugins` path is valid for Arch Linux. On other distributions, find the Qt 6 plugin path before you configure the project:

```sh
qmake6 -query QT_INSTALL_PLUGINS
```

### Install the KWin plugin

Install the plugin system-wide. This step requires root privileges.

Before you install the plugin, review the installation manifest:

```sh
cat build/install_manifest.txt
sudo cmake --install build
```

This command installs one file. It does not replace or modify a KWin binary:

```text
/usr/lib/qt6/plugins/kwin/plugins/scrollfix.so
```

### Install the settings application

Install the settings application for the current user:

```sh
cmake --install build --component Settings --prefix "$HOME/.local"
```

Add `~/.local/bin` to your `PATH` if necessary. To open the application, select **Touchpad Scroll Settings** from the application menu, or run:

```sh
scroll-fix-settings
```

### Load the plugin

Load the plugin for the current session:

```sh
qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.LoadPlugin scrollfix
```

A result of `true` indicates that KWin loaded the plugin. A result of `false` usually indicates that the plugin is already loaded. To check the loaded plugins, run:

```sh
qdbus6 org.kde.KWin /Plugins org.freedesktop.DBus.Properties.Get \
  org.kde.KWin.Plugins LoadedPlugins
```

Open the settings application. Select **Pick a window…**, click inside the application that you want to slow down, set the slider, and select **Apply**.

### Enable startup loading

Enable startup loading only after you test the plugin and review [Risks and rollback](#risks-and-rollback):

```sh
kwriteconfig6 --file kwinrc --group Plugins --key scrollfixEnabled true
```

## Configure scroll speed

Use the settings application to configure the plugin:

1. Set **Scroll speed** to a value from 5% to 100% of KDE's existing touchpad scroll speed. A value of 100% does not change the speed.
2. Select **Pick a window…**, then click inside the target application. Do not click its taskbar icon. To enter an exact Wayland application ID, select **Add app ID…**.
3. Select **Apply** to save the settings and reload the plugin for the current session.

The application list is an allowlist. Every listed application uses the same factor. Applications that are not listed keep their normal behavior.

- To restore an application's normal behavior, select **Remove**, then select **Apply**.
- To unload the plugin, select **Disable for session**.
- Closing the settings application does not make changes.
- The application saves changes only when you select **Apply**.
- The application does not change startup loading.

## Configure the plugin manually

Edit the following configuration file:

```text
~/.config/scrollfixrc
```

For example:

```ini
[Scroll]
Factor=0.7
Applications=brave-browser,chromium,google-chrome,discord,code
```

Use the exact Wayland application IDs that KWin reports. The example IDs might not match your installed packages. Electron applications can require separate entries.

The plugin uses a factor of `1.0`, which does not change scroll speed, when any of the following conditions apply:

- `Factor` is outside the range `0.05..1.0`.
- `Factor` is not finite.
- The application is not listed.

The plugin reads the configuration file when it loads. Unload and reload the plugin after you edit the file.

## Risks and rollback

KDE Scroll Fix runs inside the compositor. A crash can terminate your desktop session. Save your work before you load the plugin for the first time. Do not enable startup loading until you test the rollback procedure.

To disable startup loading and unload the plugin, run:

```sh
kwriteconfig6 --file kwinrc --group Plugins --key scrollfixEnabled false
qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.UnloadPlugin scrollfix
```

Unloading the plugin removes its input filter and immediately restores normal handling. If the desktop does not start, run the `kwriteconfig6` command from a TTY before you log in.

To uninstall the plugin, delete the `.so` file listed in `build/install_manifest.txt`.

Rebuild and test the plugin after every KWin upgrade. If the plugin no longer compiles or loads, disable it until the source is updated.

## How the plugin handles input

The plugin scales `delta` for `Finger`-source pointer-axis events only when the pointer's recipient surface belongs to a listed application ID.

The plugin passes all other events through unchanged, including:

- Mouse-wheel and v120 events
- Event source and timestamps
- Scroll-stop events and pointer frames
- Pinch and swipe events

The plugin uses the application under the pointer instead of keyboard focus. As a result, it supports hover-scrolling in an inactive window. If KWin cannot resolve a popup or subsurface to a window, the plugin leaves the event unchanged.

Lock-screen, effects, decoration, and window-action filters run before this plugin.

## Known limitations

Automated tests cover factor validation, application matching, and an offscreen settings-application startup check. They do not cover D-Bus Apply or Disable operations, compositor input routing, or gestures.

The plugin has been manually tested with only my machine. Before relying on the plugin, test the following scenarios on your system:

- A listed application compared with Firefox or Konsole
- Hover-scrolling in an unfocused window
- Popups and menus
- A physical mouse wheel
- Horizontal scrolling
- Very small or slow deltas
- Finger-lift inertia
- Pinch-to-zoom
- KDE touchpad gestures
- Unloading the plugin during scrolling

Very small deltas can lose precision because of Wayland fixed-point representation.

## License

Open source under MIT license
