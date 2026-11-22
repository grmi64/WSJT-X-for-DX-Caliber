#ifndef WS_SUITE_PREFERENCES_HPP
#define WS_SUITE_PREFERENCES_HPP

#include <QDataStream>
#include <QMetaEnum>
#include <QSettings>
#include <QVariantMap>

#include "models/DecodeHighlightingModel.hpp"

namespace WsSuitePreferences
{
  inline auto palette (QString const& encoded, DecodeHighlightingModel::HighlightItems const& current)
    -> DecodeHighlightingModel::HighlightItems
  {
    using Model = DecodeHighlightingModel;
    if (encoded.isEmpty () || encoded.size () > 16000) return {};
    auto const bytes = QByteArray::fromBase64 (encoded.toLatin1 ());
    QDataStream stream {bytes};
    quint32 count {};
    stream >> count;
    if (!count || count > 64) return {};

    // WS serializes NUL-terminated names, an option byte and two fixed-size brushes.
    // Only solid and empty brushes map to the WSJT-X palette.
    auto read_brush = [&stream] (QBrush& brush) {
      QByteArray record (84, Qt::Uninitialized);
      if (stream.readRawData (record.data (), record.size ()) != record.size ()) return false;
      auto const style = static_cast<quint8> (record.at (0));
      if (style > Qt::SolidPattern || static_cast<quint8> (record.at (1)) > QColor::ExtendedRgb)
        return false;
      QDataStream color_stream {record.mid (1, 11)};
      QColor color;
      color_stream >> color;
      if (color_stream.status () != QDataStream::Ok || (style && !color.isValid ())) return false;
      brush = style ? QBrush {color} : QBrush {};
      return true;
    };

    auto items = current;
    auto const names = QMetaEnum::fromType<Model::Highlight> ();
    for (quint32 i = 0; i < count; ++i)
      {
        quint32 length {};
        stream >> length;
        if (length < 2 || length > 48) return {};
        QByteArray name (static_cast<int> (length), Qt::Uninitialized);
        if (stream.readRawData (name.data (), name.size ()) != name.size ()
            || name.back () != '\0' || stream.skipRawData (1) != 1) return {};
        QBrush foreground, background;
        if (!read_brush (foreground) || !read_brush (background)) return {};
        bool recognized {};
        auto const type = names.keyToValue (name.constData (), &recognized);
        if (!recognized) continue;
        for (auto& item : items)
          if (static_cast<int> (item.type_) == type)
            {
              item.foreground_ = foreground;
              item.background_ = background;
              break;
            }
      }
    if (!stream.atEnd () || stream.status () != QDataStream::Ok) return {};
    return items;
  }

  inline QVariantMap read (QSettings const& source, QSettings const& target, bool& palette_skipped)
  {
    QVariantMap result;
    auto const copy_booleans = [&] (QString const& group, QStringList const& keys) {
      for (auto const& key : keys)
        {
          auto const path = group + QLatin1Char ('/') + key;
          auto const value = source.value (path).toString ().trimmed ().toLower ();
          if (value == "true" || value == "false") result.insert (path, value == "true");
        }
    };
    copy_booleans ("Configuration",
      {"Highlight73", "HighlightAllMessages", "HighlightByMode", "HighlightBlue", "HighlightOrange", "DXCCEntity",
       "MapGridEntity", "MapGridAllEntity", "AlwaysShowCountryNames", "PrincipalPrefix",
       "Blacklisted", "Whitelisted", "FiltersForWord2", "FiltersForWaitAndPounceOnly",
       "PromptToLog", "AutoLog", "ContestingOnly", "ZZ00", "Log4digitGrids", "toRTTY",
       "dBtoComments", "specOptoComments"});
    copy_booleans ("Common",
      {"BypassFilters", "HighlightB4", "HighlightToday", "HighlightIgnored",
       "HideB4", "HideToday", "HideIgnored", "IgnoreB4", "IgnoreToday", "IgnoreIgnored",
       "HideEU", "HideNA", "HideSA", "HideAS", "HideAF", "HideOC", "HideAN",
       "HighlightWhitelistEntries"});

    if (source.contains ("Configuration/HighlightAll") && !source.contains ("Configuration/HighlightAllMessages"))
      {
        auto const value = source.value ("Configuration/HighlightAll").toString ().trimmed ().toLower ();
        if (value == "true" || value == "false")
          result.insert ("Configuration/HighlightAllMessages", value == "true");
      }

    QStringList text_keys {"HighlightBlueCallsigns", "HighlightOrangeCallsigns"};
    for (int i = 1; i <= 12; ++i)
      {
        auto const suffix = QString::number (i);
        text_keys << "Blacklist" + suffix << "Whitelist" + suffix << "Pass" + suffix;
        if (i <= 4)
          {
            text_keys << "Territory" + suffix;
            copy_booleans ("Common", {"HideTerritory" + suffix, "HighlightTerritory" + suffix});
          }
      }
    for (auto const& key : text_keys)
      {
        auto const path = "Configuration/" + key;
        if (source.contains (path))
          {
            auto const value = source.value (path);
            auto const text = value.type () == QVariant::StringList
                ? value.toStringList ().join (QStringLiteral (", ")) : value.toString ();
            if (text.size () <= 4096) result.insert (path, text);
          }
      }

    auto const color_key = QStringLiteral ("Configuration/DecodeHighlighting");
    auto const ws_color_key = color_key + QStringLiteral ("_v2");
    palette_skipped = false;
    if (source.contains (ws_color_key))
      {
        auto current = target.value (color_key).value<DecodeHighlightingModel::HighlightItems> ();
        if (current.isEmpty ()) current = DecodeHighlightingModel::default_items ();
        auto const colors = palette (source.value (ws_color_key).toString (), current);
        palette_skipped = colors.isEmpty ();
        if (!palette_skipped) result.insert (color_key, QVariant::fromValue (colors));
      }
    else if (source.contains (color_key))
      {
        auto const colors = source.value (color_key).value<DecodeHighlightingModel::HighlightItems> ();
        palette_skipped = colors.isEmpty ();
        if (!palette_skipped) result.insert (color_key, QVariant::fromValue (colors));
      }

    // Import a logging choice as a pair so a destination's old AutoLog value
    // cannot silently override the user's selected prompt preference.
    auto const prompt = QStringLiteral ("Configuration/PromptToLog");
    auto const automatic = QStringLiteral ("Configuration/AutoLog");
    if (result.value (automatic).toBool ()) result.insert (prompt, false);
    else if (result.value (prompt).toBool ()) result.insert (automatic, false);
    return result;
  }

  inline QString const pending_key = QStringLiteral ("DXC/PendingWsSuitePreferences");

  inline void apply_pending (QSettings& settings)
  {
    auto const pending = settings.value (pending_key).toMap ();
    if (pending.isEmpty ()) return;
    for (auto it = pending.cbegin (); it != pending.cend (); ++it)
      settings.setValue (it.key (), it.value ());
    settings.remove (pending_key);
    settings.sync ();
  }
}
#endif
