#pragma once

#include <QStyledItemDelegate>

class QPixmap;
class ThumbnailService;

class TextureItemDelegate final : public QStyledItemDelegate
{
    Q_OBJECT
  public:
    explicit TextureItemDelegate(ThumbnailService &thumbnailService, QObject *parent = nullptr);
    ~TextureItemDelegate() override = default;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

  private:
    QPixmap LoadThumbnail(const QString &filePath) const;
    ThumbnailService &mThumbnailService;
};
