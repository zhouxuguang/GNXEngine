#include "SceneDocument.h"
#include "CommandHistory.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QUuid>
#include <algorithm>

SceneDocument::SceneDocument(CommandHistory &history, QObject *parent)
    : QObject(parent), mHistory(history)
{
    connect(&mHistory, &CommandHistory::StateChanged, this,
            [this]
            {
                SetDirty(!mHistory.IsClean());
            });
}

bool SceneDocument::NewDocument(const QString &path)
{
    mPath = QFileInfo(path).absoluteFilePath();
    mEntities = {{QUuid::createUuid().toString(QUuid::WithoutBraces), "Camera"},
                 {QUuid::createUuid().toString(QUuid::WithoutBraces), "Directional Light"}};
    mHistory.Clear();
    mHistory.InvalidateClean();
    emit Changed();
    return true;
}

bool SceneDocument::Load(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        if (error)
            *error = tr("无法读取场景文件：%1").arg(path);
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !json.isObject())
    {
        if (error)
            *error = tr("场景文件格式无效：%1").arg(parseError.errorString());
        return false;
    }
    const QJsonObject root = json.object();
    if (root.value("version").toInt(-1) != 1 || !root.value("entities").isArray())
    {
        if (error)
            *error = tr("不支持的场景文件版本或缺少实体列表");
        return false;
    }
    QVector<SceneEntity> entities;
    QSet<QString> entityIds;
    for (const QJsonValue &value : root.value("entities").toArray())
    {
        const QJsonObject object = value.toObject();
        SceneEntity entity{object.value("id").toString(), object.value("name").toString()};
        if (entity.id.isEmpty() || entity.name.isEmpty() || entityIds.contains(entity.id))
        {
            if (error)
                *error = tr("场景包含无效或重复实体");
            return false;
        }
        entityIds.insert(entity.id);
        entities.push_back(std::move(entity));
    }
    mPath = QFileInfo(path).absoluteFilePath();
    mEntities = std::move(entities);
    mHistory.Clear();
    mHistory.SetClean();
    emit Changed();
    return true;
}

bool SceneDocument::Save(QString *error)
{
    return SaveAs(mPath, error);
}

bool SceneDocument::SaveAs(const QString &path, QString *error)
{
    if (path.isEmpty())
    {
        if (error)
            *error = tr("场景保存路径为空");
        return false;
    }
    QJsonArray entities;
    for (const SceneEntity &entity : mEntities)
        entities.push_back(QJsonObject{{"id", entity.id}, {"name", entity.name}});
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
    {
        if (error)
            *error = tr("无法创建场景目录：%1").arg(path);
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(QJsonDocument(QJsonObject{{"version", 1}, {"entities", entities}})
                       .toJson(QJsonDocument::Indented)) < 0 ||
        !file.commit())
    {
        if (error)
            *error = tr("场景文件写入失败：%1").arg(path);
        return false;
    }
    mPath = QFileInfo(path).absoluteFilePath();
    mHistory.SetClean();
    return true;
}

void SceneDocument::Close()
{
    mHistory.Clear();
    mEntities.clear();
    mPath.clear();
    emit Changed();
}

int SceneDocument::IndexOf(const QString &id) const
{
    for (int index = 0; index < mEntities.size(); ++index)
        if (mEntities[index].id == id)
            return index;
    return -1;
}

const SceneEntity *SceneDocument::Find(const QString &id) const
{
    const int index = IndexOf(id);
    return index >= 0 ? &mEntities[index] : nullptr;
}

void SceneDocument::AddEntity(const QString &name)
{
    const SceneEntity entity{QUuid::createUuid().toString(QUuid::WithoutBraces),
                             name.trimmed().isEmpty() ? tr("Entity") : name.trimmed()};
    mHistory.Execute({tr("添加 %1").arg(entity.name),
                      [this, entity]
                      {
                          mEntities.push_back(entity);
                          emit Changed();
                      },
                      [this, entity]
                      {
                          const int i = IndexOf(entity.id);
                          if (i >= 0)
                              mEntities.removeAt(i);
                          emit Changed();
                      }});
}

void SceneDocument::RemoveEntity(const QString &id)
{
    const int index = IndexOf(id);
    if (index < 0)
        return;
    const SceneEntity entity = mEntities[index];
    mHistory.Execute({tr("删除 %1").arg(entity.name),
                      [this, id]
                      {
                          const int i = IndexOf(id);
                          if (i >= 0)
                              mEntities.removeAt(i);
                          emit Changed();
                      },
                      [this, index, entity]
                      {
                          mEntities.insert(std::min(index, static_cast<int>(mEntities.size())),
                                           entity);
                          emit Changed();
                      }});
}

void SceneDocument::RenameEntity(const QString &id, const QString &name)
{
    const int index = IndexOf(id);
    const QString trimmed = name.trimmed();
    if (index < 0 || trimmed.isEmpty() || mEntities[index].name == trimmed)
        return;
    const QString previous = mEntities[index].name;
    auto rename = [this, id](const QString &value)
    {
        const int i = IndexOf(id);
        if (i >= 0)
            mEntities[i].name = value;
        emit Changed();
    };
    mHistory.Execute({tr("重命名实体"),
                      [rename, trimmed]
                      {
                          rename(trimmed);
                      },
                      [rename, previous]
                      {
                          rename(previous);
                      }});
}

void SceneDocument::SetDirty(bool dirty)
{
    if (mDirty == dirty)
        return;
    mDirty = dirty;
    emit DirtyChanged(dirty);
}
