#pragma once

#include <QObject>
#include <QString>

class SelectionService final : public QObject
{
    Q_OBJECT
  public:
    explicit SelectionService(QObject *parent = nullptr);
    const QString &SelectedId() const
    {
        return mSelectedId;
    }
    void Select(const QString &id);
    void Clear();

  signals:
    void SelectionChanged(const QString &id);

  private:
    QString mSelectedId;
};
