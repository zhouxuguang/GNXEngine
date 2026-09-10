#pragma once

#include <QObject>
#include <QString>
#include <functional>
#include <vector>

class CommandHistory final : public QObject
{
    Q_OBJECT
  public:
    struct Command
    {
        QString text;
        std::function<void()> redo;
        std::function<void()> undo;
    };

    explicit CommandHistory(QObject *parent = nullptr);
    void Execute(Command command);
    void Undo();
    void Redo();
    void Clear();
    void SetClean();
    void InvalidateClean();
    bool CanUndo() const
    {
        return mCursor > 0;
    }
    bool CanRedo() const
    {
        return mCursor < mCommands.size();
    }
    bool IsClean() const
    {
        return mCleanCursor == mCursor;
    }

  signals:
    void StateChanged();

  private:
    std::vector<Command> mCommands;
    size_t mCursor = 0;
    size_t mCleanCursor = 0;
};
