#include <input.h>
#include <input_event.h>
#include <plugin.h>
#include <wayland/seat.h>
#include <wayland_server.h>
#include <window.h>

#include <KConfigGroup>
#include <KSharedConfig>

#include "policy.h"

class ScrollPlugin final : public KWin::Plugin, public KWin::InputEventFilter {
 public:
  ScrollPlugin()
      // After decorations and window actions, before normal client forwarding.
      : InputEventFilter(KWin::InputFilterOrder::InputMethod) {
    const KConfigGroup config(
        KSharedConfig::openConfig(QStringLiteral("scrollfixrc")),
        QStringLiteral("Scroll"));
    m_factor = config.readEntry("Factor", 1.0);
    m_apps = config.readEntry("Applications", QStringList{});
    KWin::input()->installInputEventFilter(this);
  }

  bool pointerAxis(KWin::PointerAxisEvent* event) override {
    if (event->source != KWin::PointerAxisSource::Finger) {
      return false;
    }
    auto* server = KWin::waylandServer();
    // Use the actual pointer recipient, not the keyboard-active window.
    auto* surface = server->seat()->focusedPointerSurface();
    if (!surface) {
      return false;
    }
    auto* window = server->findWindow(surface);
    if (window) {
      event->delta *=
          ScrollFix::factorFor(window->resourceClass(), m_apps, m_factor);
    }
    return false;
  }

 private:
  double m_factor = 1.0;
  QStringList m_apps;
};

class ScrollFactory final : public KWin::PluginFactory {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID PluginFactory_iid FILE "metadata.json")
  Q_INTERFACES(KWin::PluginFactory)
 public:
  std::unique_ptr<KWin::Plugin> create() const override {
    return std::make_unique<ScrollPlugin>();
  }
};

#include "plugin.moc"
