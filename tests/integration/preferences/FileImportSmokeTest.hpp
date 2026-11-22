#ifndef FILE_IMPORT_SMOKE_TEST_HPP
#define FILE_IMPORT_SMOKE_TEST_HPP

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

#include "WsSuitePreferences.hpp"

namespace FileImportSmokeTest
{
  inline auto make_source (QString const& path, bool ws_suite) -> bool
  {
    using Model = DecodeHighlightingModel;
    QSettings source {path, QSettings::IniFormat};
    source.clear ();
    source.setValue ("Configuration/Rig", "Fixture-only rig");
    source.setValue ("Configuration/UDPServer", "127.0.0.2");
    source.setValue ("Configuration/AdifLogFile", "/fixture-only.adi");
    source.setValue ("Configuration/Territory1", "Japan");
    source.setValue ("Common/HideTerritory1", true);
    source.setValue (ws_suite ? "Configuration/PromptToLog" : "Configuration/AutoLog", true);

    QBrush const color {QColor {"#127578"}};
    if (ws_suite)
      {
        QByteArray bytes;
        QDataStream stream {&bytes, QIODevice::WriteOnly};
        stream << quint32 {1} << "MyCall" << quint8 {1} << QBrush {} << color;
        source.setValue ("Configuration/DecodeHighlighting_v2", QString::fromLatin1 (bytes.toBase64 ()));
      }
    else
      {
        auto palette = Model::default_items ();
        for (auto& item : palette)
          if (item.type_ == Model::Highlight::MyCall) item.background_ = color;
        source.setValue ("Configuration/DecodeHighlighting", QVariant::fromValue (palette));
      }
    source.sync ();
    return source.status () == QSettings::NoError;
  }

  inline QString check (QSettings const& settings, bool ws_suite, bool applied)
  {
    using Model = DecodeHighlightingModel;
    if (settings.value ("Configuration/Rig").toString () != "None"
        || settings.value ("Configuration/UDPServer").toString () != "127.0.0.1"
        || !settings.value ("Configuration/AdifLogFile").toString ().isEmpty ())
      return QStringLiteral ("Import altered the DXC connection settings or log path.");

    if (settings.value ("Common/HideTerritory1").toBool () != applied
        || settings.value ("Configuration/Territory1").toString () != (applied ? "Japan" : "")
        || settings.value ("Configuration/AutoLog").toBool () != (applied && !ws_suite)
        || settings.value ("Configuration/PromptToLog").toBool () != (applied && ws_suite))
      return QStringLiteral ("The imported filter or logging choice was incorrect.");

    auto const palette = settings.value ("Configuration/DecodeHighlighting",
      QVariant::fromValue (Model::default_items ())).value<Model::HighlightItems> ();
    for (auto const& item : palette)
      if (item.type_ == Model::Highlight::MyCall
          && (item.background_.color () == QColor {"#127578"}) == applied)
        return {};
    return QStringLiteral ("The imported My Call color was incorrect.");
  }

  inline QString run (QWidget& window, QSettings& settings, QString const& directory,
                      bool ws_suite, bool verify)
  {
    if (verify)
      {
        if (settings.contains (WsSuitePreferences::pending_key))
          return QStringLiteral ("The staged import was not consumed on restart.");
        return check (settings, ws_suite, true);
      }

    auto * menu = window.findChild<QMenu *> ("menuFile");
    if (!menu) return QStringLiteral ("File menu not found.");
    QAction * import = nullptr;
    for (auto * action : menu->actions ())
      if (action->text () == "Import WS Suite or WSJT-X Preferences...") import = action;
    if (!import || !import->isEnabled () || !import->isVisible ())
      return QStringLiteral ("File menu import action is unavailable.");

    auto const path = directory + (ws_suite ? "/WS.ini" : "/WSJT-X.ini");
    if (!make_source (path, ws_suite)) return QStringLiteral ("Could not write the source profile.");
    QFile source {path};
    if (!source.open (QIODevice::ReadOnly)) return QStringLiteral ("Could not read the source profile.");
    auto const original = source.readAll ();
    source.close ();

    for (auto button : {QMessageBox::No, QMessageBox::Yes})
      {
        bool selected = false;
        bool confirmed = false;
        int ticks = 0;
        QTimer dialog_driver;
        QObject::connect (&dialog_driver, &QTimer::timeout, &window, [&] {
          auto * modal = QApplication::activeModalWidget ();
          if (++ticks > 150)
            {
              if (auto * dialog = qobject_cast<QDialog *> (modal)) dialog->reject ();
              return;
            }
          if (!selected)
            {
              if (auto * picker = qobject_cast<QFileDialog *> (modal))
                {
                  auto * edit = picker->findChild<QLineEdit *> ("fileNameEdit");
                  if (!edit) return;
                  edit->setText (path);
                  if (!picker->selectedFiles ().contains (path)) return;
                  selected = true;
                  QMetaObject::invokeMethod (picker, "accept", Qt::DirectConnection);
                }
            }
          else if (auto * prompt = qobject_cast<QMessageBox *> (modal))
            {
              auto * choice = prompt->button (button);
              if (choice && prompt->text () == "Import Preferences")
                {
                  confirmed = true;
                  choice->click ();
                }
              else
                prompt->reject ();
            }
        });
        dialog_driver.start (20);
        import->trigger ();
        dialog_driver.stop ();
        if (!selected) return QStringLiteral ("Could not select the fixture in the File import chooser.");
        if (!confirmed) return QStringLiteral ("File import did not show its confirmation.");
        if (settings.contains (WsSuitePreferences::pending_key) != (button == QMessageBox::Yes))
          return QStringLiteral ("The confirmation did not control staging of the import.");
        auto const unchanged = check (settings, ws_suite, false);
        if (!unchanged.isEmpty ()) return unchanged;
      }
    if (!source.open (QIODevice::ReadOnly) || source.readAll () != original)
      return QStringLiteral ("The import modified the source profile.");
    settings.sync ();
    return settings.status () == QSettings::NoError
      ? QString {} : QStringLiteral ("Could not persist the staged import.");
  }
}
#endif
