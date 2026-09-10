#include "TextureItemDelegate.h"
#include "ThumbnailService.h"
#include <QFileInfo>
#include <QFileSystemModel>
#include <QPainter>
#include <QSortFilterProxyModel>

TextureItemDelegate::TextureItemDelegate(ThumbnailService &thumbnailService, QObject *parent)
    : QStyledItemDelegate(parent), mThumbnailService(thumbnailService)
{
}

void TextureItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    const QSortFilterProxyModel *proxyModel =
        qobject_cast<const QSortFilterProxyModel *>(index.model());
    if (!proxyModel)
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QFileSystemModel *fileModel = qobject_cast<QFileSystemModel *>(proxyModel->sourceModel());
    if (!fileModel)
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QModelIndex sourceIndex = proxyModel->mapToSource(index);

    QString filePath = fileModel->filePath(sourceIndex);
    QFileInfo fileInfo(filePath);

    if (fileInfo.isDir())
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QString fileName = fileInfo.fileName();
    QString suffix = fileInfo.suffix().toLower();

    bool isImageFile = (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || suffix == "bmp" ||
                        suffix == "tga" || suffix == "webp" || suffix == "hdr" || suffix == "exr");

    if (isImageFile)
    {
        QPixmap thumbnail = LoadThumbnail(filePath);

        if (!thumbnail.isNull())
        {
            painter->save();

            if (option.state & QStyle::State_Selected)
            {
                painter->fillRect(option.rect, option.palette.highlight());
            }
            else
            {
                painter->fillRect(option.rect, option.palette.base());
            }

            QSize iconSize = option.decorationSize;
            QRect iconRect(QPoint(0, 0), iconSize);
            iconRect.moveTop(option.rect.top() + 6);
            iconRect.moveCenter(QPoint(option.rect.center().x(), iconRect.center().y()));

            painter->drawPixmap(iconRect, thumbnail);

            QRect textRect = option.rect;
            textRect.setTop(iconRect.bottom() + 4);
            textRect.setHeight(option.rect.height() - iconRect.height() - 4);

            QString displayName = fileName;
            QFontMetrics fontMetrics(painter->font());
            QString elidedName =
                fontMetrics.elidedText(displayName, Qt::ElideRight, textRect.width());

            painter->setPen(option.palette.text().color());
            painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, elidedName);

            painter->restore();
            return;
        }
    }

    QStyledItemDelegate::paint(painter, option, index);
}

QSize TextureItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);

    return QSize(100, 120);
}

QPixmap TextureItemDelegate::LoadThumbnail(const QString &filePath) const
{
    const QImage image = mThumbnailService.Thumbnail(filePath);
    return image.isNull() ? QPixmap()
                          : QPixmap::fromImage(image).scaled(QSize(64, 64), Qt::KeepAspectRatio,
                                                             Qt::SmoothTransformation);
}
