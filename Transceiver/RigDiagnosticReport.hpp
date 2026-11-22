#ifndef RIG_DIAGNOSTIC_REPORT_HPP
#define RIG_DIAGNOSTIC_REPORT_HPP

#include <deque>

#include <QDateTime>
#include <QElapsedTimer>
#include <QMutex>
#include <QStringList>

class RigDiagnosticReport
{
public:
  static void record (QString const& event)
  {
    QMutexLocker lock {&instance ().mutex_};
    auto& report = instance ();
    auto const now = report.clock_.elapsed ();
    report.prune (now);
    if (report.events_.size () == 512) report.events_.pop_front ();
    report.events_.push_back ({now, QDateTime::currentDateTimeUtc ().toString (Qt::ISODateWithMs)
                                   + QStringLiteral ("  ") + event});
  }

  static QString snapshot ()
  {
    auto& report = instance ();
    QMutexLocker lock {&report.mutex_};
    report.prune (report.clock_.elapsed ());
    QStringList lines;
    for (auto const& event : report.events_) lines << event.text;
    return lines.join (QLatin1Char ('\n'));
  }

private:
  struct Event { qint64 elapsed; QString text; };

  static RigDiagnosticReport& instance ()
  {
    static RigDiagnosticReport report;
    return report;
  }

  RigDiagnosticReport () { clock_.start (); }

  void prune (qint64 now)
  {
    while (!events_.empty () && now - events_.front ().elapsed > 60000)
      events_.pop_front ();
  }

  QMutex mutex_;
  QElapsedTimer clock_;
  std::deque<Event> events_;
};

#endif
