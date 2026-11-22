#include "DecodeHighlightingListView.hpp"

#include <QAction>
#include <QColorDialog>
#include <QPainter>
#include <QStyle>
#include <QStyledItemDelegate>

#include "models/DecodeHighlightingModel.hpp"
#include "MessageBox.hpp"

#include "moc_DecodeHighlightingListView.cpp"

namespace
{
  class HighlightRuleDelegate final : public QStyledItemDelegate
  {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint (QPainter * painter, QStyleOptionViewItem const& option,
                QModelIndex const& index) const override
    {
      QStyledItemDelegate::paint (painter, option, index);

      QStyleOptionViewItem item {option};
      initStyleOption (&item, index);
      if (!item.features.testFlag (QStyleOptionViewItem::HasCheckIndicator) || !item.widget)
        {
          return;
        }

      // Use the same indicator rectangle Qt uses for mouse input. Draw the
      // indicator over the row swatch, since some macOS styles omit its border.
      auto const indicator = item.widget->style ()->subElementRect (
        QStyle::SE_ItemViewItemCheckIndicator, &item, item.widget);
      auto const size = qMin (indicator.width (), indicator.height ());
      if (size < 4) return;
      QRectF box {0, 0, size - 1.5, size - 1.5};
      box.moveCenter (QRectF {indicator}.center ());
      auto const checked = item.checkState == Qt::Checked;
      painter->save ();
      painter->setRenderHint (QPainter::Antialiasing);
      painter->setPen (QPen {QColor {"#182535"}, 1.25});
      painter->setBrush (checked ? QColor {"#075eb5"} : Qt::white);
      painter->drawRoundedRect (box, 2, 2);
      if (checked)
        {
          auto const x = box.x ();
          auto const y = box.y ();
          auto const width = box.width ();
          painter->setPen (QPen {Qt::white, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin});
          QPointF const tick[] {{x + .22 * width, y + .50 * width},
                                {x + .43 * width, y + .71 * width},
                                {x + .80 * width, y + .29 * width}};
          painter->drawPolyline (tick, 3);
        }
      painter->restore ();
    }
  };
}

DecodeHighlightingListView::DecodeHighlightingListView (QWidget * parent)
  : QListView {parent}
{
  setItemDelegate (new HighlightRuleDelegate {this});

  auto * fg_colour_action = new QAction {tr ("&Foreground color ..."), this};
  addAction (fg_colour_action);
  connect (fg_colour_action, &QAction::triggered, [this] (bool /*checked*/) {
      auto const& index = currentIndex ();
      if (!model () || !index.isValid ())
        {
          return;
        }
      auto colour = QColorDialog::getColor (model ()->data (index, Qt::ForegroundRole).value<QBrush> ().color ()
                                            , this
                                            , tr ("Choose %1 Foreground Color")
                                                .arg (model ()->data (index).toString ()));
      if (colour.isValid ())
        {
          model ()->setData (index, QBrush {colour}, Qt::ForegroundRole);
        }
    });

  auto * unset_fg_colour_action = new QAction {tr ("&Unset foreground color"), this};
  addAction (unset_fg_colour_action);
  connect (unset_fg_colour_action, &QAction::triggered, [this] (bool /*checked*/) {
      auto const& index = currentIndex ();
      if (model () && index.isValid ())
        {
          model ()->setData (index, QBrush {}, Qt::ForegroundRole);
        }
    });

  auto * bg_colour_action = new QAction {tr ("&Background color ..."), this};
  addAction (bg_colour_action);
  connect (bg_colour_action, &QAction::triggered, [this] (bool /*checked*/) {
      auto const& index = currentIndex ();
      if (!model () || !index.isValid ())
        {
          return;
        }
      auto colour = QColorDialog::getColor (model ()->data (index, Qt::BackgroundRole).value<QBrush> ().color ()
                                            , this
                                            , tr ("Choose %1 Background Color")
                                                .arg (model ()->data (index).toString ()));
      if (colour.isValid ())
        {
          model ()->setData (index, QBrush {colour}, Qt::BackgroundRole);
        }
    });

  auto * unset_bg_colour_action = new QAction {tr ("U&nset background color"), this};
  addAction (unset_bg_colour_action);
  connect (unset_bg_colour_action, &QAction::triggered, [this] (bool /*checked*/) {
      auto const& index = currentIndex ();
      if (model () && index.isValid ())
        {
          model ()->setData (index, QBrush {}, Qt::BackgroundRole);
        }
    });

  auto * defaults_action = new QAction {tr ("&Reset this item to Default 1"), this};
  addAction (defaults_action);
  connect (defaults_action, &QAction::triggered, [this] (bool /*checked*/) {
      auto const& index = currentIndex ();
      if (!model () || !index.isValid ())
        {
          return;
        }
      model ()->setData (index, model ()->data (index, DecodeHighlightingModel::EnabledDefaultRole).toBool () ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
      model ()->setData (index, model ()->data (index, DecodeHighlightingModel::ForegroundDefaultRole), Qt::ForegroundRole);
      model ()->setData (index, model ()->data (index, DecodeHighlightingModel::BackgroundDefaultRole), Qt::BackgroundRole);
    });
}

QSize DecodeHighlightingListView::sizeHint () const
{
  auto item_height = sizeHintForRow (0);
  if (item_height >= 0)
    {
      // set the height hint to exactly the space required for all the
      // items
      return {width (), (model ()->rowCount () * (item_height + 2 * spacing ())) + 2 * frameWidth ()};
    }
  return QListView::sizeHint ();
}
