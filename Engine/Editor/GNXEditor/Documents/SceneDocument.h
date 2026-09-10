#pragma once

#include <QObject>
#include <QString>
#include <QVector>

class CommandHistory;

struct SceneEntity
{
    QString id;
    QString name;
};

class SceneDocument final : public QObject
{
    Q_OBJECT
  public:
    explicit SceneDocument(CommandHistory &history, QObject *parent = nullptr);
    bool NewDocument(const QString &path);
    bool Load(const QString &path, QString *error = nullptr);
    bool Save(QString *error = nullptr);
    bool SaveAs(const QString &path, QString *error = nullptr);
    void Close();
    void AddEntity(const QString &name);
    void RemoveEntity(const QString &id);
    void RenameEntity(const QString &id, const QString &name);
    const QVector<SceneEntity> &Entities() const
    {
        return mEntities;
    }
    const SceneEntity *Find(const QString &id) const;
    bool IsOpen() const
    {
        return !mPath.isEmpty();
    }
    bool IsDirty() const
    {
        return mDirty;
    }
    const QString &Path() const
    {
        return mPath;
    }

  signals:
    void Changed();
    void DirtyChanged(bool dirty);

  private:
    void SetDirty(bool dirty);
    int IndexOf(const QString &id) const;
    CommandHistory &mHistory;
    QVector<SceneEntity> mEntities;
    QString mPath;
    bool mDirty = false;
};
