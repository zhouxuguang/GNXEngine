#include "TextureItemDelegate.h"
#include <QPainter>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QSortFilterProxyModel>
#include "Runtime/AssetProcess/include/TextureMetaFormat.h"

TextureItemDelegate::TextureItemDelegate(QObject* parent)
	: QStyledItemDelegate(parent)
{
}

TextureItemDelegate::~TextureItemDelegate()
{
}

void TextureItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	// 尝试获取代理模型
	const QSortFilterProxyModel* proxyModel = qobject_cast<const QSortFilterProxyModel*>(index.model());
	if (!proxyModel)
	{
		QStyledItemDelegate::paint(painter, option, index);
		return;
	}

	// 从代理模型获取源模型
	QFileSystemModel* fileModel = qobject_cast<QFileSystemModel*>(proxyModel->sourceModel());
	if (!fileModel)
	{
		QStyledItemDelegate::paint(painter, option, index);
		return;
	}

	// 映射到源模型索引
	QModelIndex sourceIndex = proxyModel->mapToSource(index);

	QString filePath = fileModel->filePath(sourceIndex);
	QFileInfo fileInfo(filePath);

	// 如果是目录，使用默认绘制
	if (fileInfo.isDir())
	{
		QStyledItemDelegate::paint(painter, option, index);
		return;
	}

	QString fileName = fileInfo.fileName();
	QString suffix = fileInfo.suffix().toLower();

	// 检查是否为图像文件
	bool isImageFile = (suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
	                    suffix == "bmp" || suffix == "tga" || suffix == "webp" || suffix == "hdr");

	if (isImageFile)
	{
		// 尝试加载缩略图
		QPixmap thumbnail = LoadThumbnail(filePath);

		if (!thumbnail.isNull())
		{
			// 绘制缩略图
			painter->save();

			// 绘制背景
			if (option.state & QStyle::State_Selected)
			{
				painter->fillRect(option.rect, option.palette.highlight());
			}
			else
			{
				painter->fillRect(option.rect, option.palette.base());
			}

			// 计算缩略图位置（居中）
			QSize iconSize = option.decorationSize;
			QRect iconRect = option.rect;
			iconRect.setWidth(iconSize.width());
			iconRect.setHeight(iconSize.height());
			iconRect.moveCenter(option.rect.center());

			// 绘制缩略图
			painter->drawPixmap(iconRect, thumbnail);

			// 绘制文件名
			QRect textRect = option.rect;
			textRect.setTop(iconRect.bottom() + 4);
			textRect.setHeight(option.rect.height() - iconRect.height() - 4);

			QString displayName = fileName;
			QFontMetrics fontMetrics(painter->font());
			QString elidedName = fontMetrics.elidedText(displayName, Qt::ElideRight, textRect.width());

			painter->setPen(option.palette.text().color());
			painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, elidedName);

			painter->restore();
			return;
		}
	}

	// 如果无法加载缩略图或非图像文件，使用默认绘制
	QStyledItemDelegate::paint(painter, option, index);
}

QSize TextureItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QSize size = QStyledItemDelegate::sizeHint(option, index);

	// 返回较大的尺寸以容纳缩略图和文本
	return QSize(100, 120);
}

QPixmap TextureItemDelegate::LoadThumbnail(const QString& filePath) const
{
	// 检查缓存
	if (mThumbnailCache.contains(filePath))
	{
		return mThumbnailCache[filePath];
	}

	QFileInfo fileInfo(filePath);
	QString suffix = fileInfo.suffix().toLower();

	// 只处理图像文件
	if (suffix != "png" && suffix != "jpg" && suffix != "jpeg" &&
	    suffix != "bmp" && suffix != "tga" && suffix != "webp" && suffix != "hdr")
	{
		return QPixmap();
	}

	// 获取 meta 文件
	QString metaFilePath = filePath + ".meta";
	QFile metaFile(metaFilePath);

	if (!metaFile.exists())
	{
		return QPixmap();
	}

	// 读取 hash
	uint64_t hash = GetHashFromMetaFile(metaFilePath);
	if (hash == 0)
	{
		return QPixmap();
	}

	// 获取缩略图路径
	// 假设文件在 Assets 目录下，项目根目录是 Assets 的父目录
	QString assetsDir = fileInfo.absolutePath();
	QString projectRoot = QDir(assetsDir).absolutePath(); // 先使用文件所在目录
	fs::path path = AssetProcess::TextureImporter::GetThumbnailFilePath(hash, projectRoot.toStdString());
	path = path.lexically_normal();
	QString thumbnailPath = QString::fromStdString(path.string());

	// 如果缩略图不存在，尝试使用项目根目录（Assets 的父目录）
	if (!QFile::exists(thumbnailPath))
	{

		QDir dir(assetsDir);
		dir.cdUp(); // 转到父目录
		projectRoot = dir.absolutePath();
		thumbnailPath = QString::fromStdString(AssetProcess::TextureImporter::GetThumbnailFilePath(hash, projectRoot.toStdString()));
	}

	// 加载缩略图
	QPixmap thumbnail(thumbnailPath);

	if (thumbnail.isNull())
	{
		qDebug() << "Failed to load thumbnail:" << thumbnailPath;
		return QPixmap();
	}

	// 缩放到图标大小
	QSize iconSize(64, 64);
	thumbnail = thumbnail.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	// 缓存缩略图
	mThumbnailCache[filePath] = thumbnail;
	mHashCache[filePath] = hash;

	return thumbnail;
}

uint64_t TextureItemDelegate::GetHashFromMetaFile(const QString& filePath) const
{
	// 检查缓存
	if (mHashCache.contains(filePath))
	{
		return mHashCache[filePath];
	}

	// 使用 TextureMetaSerializer 加载 meta 文件
	AssetProcess::TextureMeta meta;
	if (AssetProcess::TextureMetaSerializer::LoadFromYAML(meta, filePath.toStdString()))
	{
		// 缓存 hash
		mHashCache[filePath] = meta.sourceFileHash;
		return meta.sourceFileHash;
	}
	else
	{
		qDebug() << "Failed to load meta file:" << filePath;
	}

	return 0;
}
