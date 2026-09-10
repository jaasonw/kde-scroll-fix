#include <KConfigGroup>
#include <KSharedConfig>
#include <QApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "policy.h"

namespace {
const QString pluginId = QStringLiteral("scrollfix");

class SettingsWindow final : public QWidget {
 public:
  SettingsWindow() {
    setWindowTitle(tr("Touchpad Scroll Settings"));
    resize(480, 420);
    auto* layout = new QVBoxLayout(this);
    auto* intro = new QLabel(
        tr("One scroll speed for all listed apps. Other apps stay unchanged."));
    intro->setWordWrap(true);
    layout->addWidget(intro);

    m_speed = new QSlider(Qt::Horizontal);
    m_speed->setRange(5, 100);
    m_speed->setAccessibleName(tr("Scroll speed percentage"));
    auto* speedLabel = new QLabel(tr("&Scroll speed:"));
    speedLabel->setBuddy(m_speed);
    layout->addWidget(speedLabel);
    layout->addWidget(m_speed);
    m_percentage = new QLabel;
    m_percentage->setTextFormat(Qt::PlainText);
    layout->addWidget(m_percentage);
    connect(m_speed, &QSlider::valueChanged, this, [this](int value) {
      m_percentage->setText(
          tr("%1% of KDE scroll speed (100% = unchanged)").arg(value));
    });

    m_apps = new QListWidget;
    m_apps->setAccessibleName(tr("Application IDs"));
    layout->addWidget(m_apps);
    auto* buttons = new QHBoxLayout;
    auto* pick = new QPushButton(tr("&Pick a window…"));
    auto* manual = new QPushButton(tr("Add app &ID…"));
    auto* remove = new QPushButton(tr("&Remove"));
    buttons->addWidget(pick);
    buttons->addWidget(manual);
    buttons->addWidget(remove);
    layout->addLayout(buttons);
    connect(pick, &QPushButton::clicked, this,
            [this, pick] { pickWindow(pick); });
    connect(manual, &QPushButton::clicked, this, [this] {
      bool ok = false;
      const auto id = QInputDialog::getText(
          this, tr("Add app ID"),
          tr("Exact Wayland app ID (for example brave-browser, chromium, or "
             "google-chrome):"),
          QLineEdit::Normal, {}, &ok);
      if (ok) {
        addApp(id);
      }
    });
    connect(remove, &QPushButton::clicked, this,
            [this] { delete m_apps->takeItem(m_apps->currentRow()); });

    m_status =
        new QLabel(tr("Apply saves settings and enables the installed plugin "
                      "for this session."));
    // App IDs and D-Bus errors land here; QLabel would auto-detect them as rich
    // text.
    m_status->setTextFormat(Qt::PlainText);
    m_status->setWordWrap(true);
    layout->addWidget(m_status);
    auto* actions = new QHBoxLayout;
    auto* disable = new QPushButton(tr("&Disable for session"));
    auto* apply = new QPushButton(tr("&Apply"));
    actions->addWidget(disable);
    actions->addWidget(apply);
    layout->addLayout(actions);
    connect(apply, &QPushButton::clicked, this, [this] { applySettings(); });
    connect(disable, &QPushButton::clicked, this, [this] {
      const bool wasLoaded = pluginLoaded();
      if (unloadPlugin()) {
        m_status->setText(
            wasLoaded
                ? tr("Disabled for this session. Stock scrolling restored.")
                : tr("Plugin was not loaded. Nothing to disable."));
      }
    });

    const KConfigGroup config(
        KSharedConfig::openConfig(QStringLiteral("scrollfixrc")),
        QStringLiteral("Scroll"));
    const auto ids = config.readEntry("Applications", QStringList{});
    for (const auto& id : ids) {
      addApp(id);
    }
    const auto factor = config.readEntry("Factor", 1.0);
    m_speed->setValue(ScrollFix::validFactor(factor) ? qRound(factor * 100)
                                                     : 100);
  }

 private:
  void addApp(const QString& value) {
    const auto id = value.trimmed();
    if (!id.isEmpty() && m_apps->findItems(id, Qt::MatchExactly).isEmpty()) {
      m_apps->addItem(id);
    }
  }

  void pickWindow(QPushButton* button) {
    m_status->setText(tr(
        "Click inside the target app, not its taskbar icon. Escape cancels."));
    button->setEnabled(false);
    QDBusInterface kwin(QStringLiteral("org.kde.KWin"), QStringLiteral("/KWin"),
                        QStringLiteral("org.kde.KWin"),
                        QDBusConnection::sessionBus());
    kwin.setTimeout(120000);
    auto* watcher = new QDBusPendingCallWatcher(
        kwin.asyncCall(QStringLiteral("queryWindowInfo")), this);
    connect(
        watcher, &QDBusPendingCallWatcher::finished, this,
        [this, button](QDBusPendingCallWatcher* call) {
          const QDBusPendingReply<QVariantMap> reply = *call;
          button->setEnabled(true);
          call->deleteLater();
          if (reply.isError()) {
            m_status->setText(
                tr("Window selection failed: %1").arg(reply.error().message()));
            return;
          }
          const auto id =
              reply.value().value(QStringLiteral("resourceClass")).toString();
          if (id.isEmpty() || id == QStringLiteral("plasmashell")) {
            m_status->setText(
                tr("No application selected. Pick inside the application's "
                   "window."));
            return;
          }
          addApp(id);
          m_status->setText(tr("Added %1. Click Apply to activate. Works with "
                               "Chromium-based and other native Wayland apps.")
                                .arg(id));
        });
  }

  // UnloadPlugin is void over D-Bus, so ask first to report honestly.
  bool pluginLoaded() {
    QDBusInterface plugins(
        QStringLiteral("org.kde.KWin"), QStringLiteral("/Plugins"),
        QStringLiteral("org.kde.KWin.Plugins"), QDBusConnection::sessionBus());
    plugins.setTimeout(3000);
    return plugins.property("LoadedPlugins").toStringList().contains(pluginId);
  }

  bool unloadPlugin() {
    QDBusInterface plugins(
        QStringLiteral("org.kde.KWin"), QStringLiteral("/Plugins"),
        QStringLiteral("org.kde.KWin.Plugins"), QDBusConnection::sessionBus());
    plugins.setTimeout(3000);
    const QDBusReply<void> reply =
        plugins.call(QStringLiteral("UnloadPlugin"), pluginId);
    if (!reply.isValid()) {
      m_status->setText(
          tr("Could not disable plugin: %1").arg(reply.error().message()));
    }
    return reply.isValid();
  }

  void applySettings() {
    QStringList ids;
    for (int i = 0; i < m_apps->count(); ++i) {
      ids.append(m_apps->item(i)->text());
    }
    auto config = KSharedConfig::openConfig(QStringLiteral("scrollfixrc"));
    config->reparseConfiguration();
    KConfigGroup group(config, QStringLiteral("Scroll"));
    group.writeEntry("Factor", m_speed->value() / 100.0);
    group.writeEntry("Applications", ids);
    if (!config->sync()) {
      QMessageBox::warning(
          this, tr("Save failed"),
          tr("Could not save settings. Running plugin was not changed."));
      return;
    }
    if (!unloadPlugin()) {
      return;
    }
    QDBusInterface plugins(
        QStringLiteral("org.kde.KWin"), QStringLiteral("/Plugins"),
        QStringLiteral("org.kde.KWin.Plugins"), QDBusConnection::sessionBus());
    plugins.setTimeout(3000);
    const QDBusReply<bool> reply =
        plugins.call(QStringLiteral("LoadPlugin"), pluginId);
    if (!reply.isValid() || !reply.value()) {
      m_status->setText(
          tr("Settings saved, but plugin loading failed. Check installation "
             "and KWin version. %1")
              .arg(reply.isValid() ? QString{} : reply.error().message()));
      return;
    }
    m_status->setText(
        tr("Applied: %1% for %2 apps. Startup loading was not changed.")
            .arg(m_speed->value())
            .arg(ids.size()));
  }

  QSlider* m_speed;
  QLabel* m_percentage;
  QListWidget* m_apps;
  QLabel* m_status;
};
}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("scroll-fix-settings"));
  QApplication::setDesktopFileName(QStringLiteral("scroll-fix-settings"));
  SettingsWindow window;
  window.show();
  if (app.arguments().contains(QStringLiteral("--smoke-test"))) {
    QTimer::singleShot(0, &app, &QApplication::quit);
  }
  return app.exec();
}
