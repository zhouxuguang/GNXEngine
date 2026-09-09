#pragma once

#include <QStyledItemDelegate>
#include <QPixmap>
#include <QHash>
#include <QFileSystemModel>
#include <QModelIndex>
#include <QSize>
#include "Runtime/AssetProcess/include/TextureMetaFormat.h"
#include "Runtime/AssetProcess/include/TextureImporter.h"

/**
 * 自定义项代理，用于在内容浏览器中显示纹理缩略图
 */
class TextureItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	explicit TextureItemDelegate(QObject* parent = nullptr);
	~TextureItemDelegate();

	// 绘制项
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	// 返回项的大小
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
	/**
	 * 加载纹理缩略图
	 * @param filePath 文件路径
	 * @return 缩略图图片
	 */
	QPixmap LoadThumbnail(const QString& filePath) const;

	/**
	 * 从 meta 文件获取 hash
	 * @param filePath 文件路径
	 * @return hash 值（失败返回 0）
	 */
	uint64_t GetHashFromMetaFile(const QString& filePath) const;

	// 缩略图缓存
	mutable QHash<QString, QPixmap> mThumbnailCache;
	mutable QHash<QString, uint64_t> mHashCache;
};
