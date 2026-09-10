#include "CommandHistory.h"
#include <limits>

CommandHistory::CommandHistory(QObject *parent) : QObject(parent)
{
}

void CommandHistory::Execute(Command command)
{
    if (!command.redo || !command.undo)
        return;
    if (mCursor < mCommands.size())
    {
        if (mCleanCursor > mCursor)
            mCleanCursor = std::numeric_limits<size_t>::max();
        mCommands.erase(mCommands.begin() + static_cast<ptrdiff_t>(mCursor), mCommands.end());
    }
    command.redo();
    mCommands.push_back(std::move(command));
    mCursor = mCommands.size();
    emit StateChanged();
}

void CommandHistory::Undo()
{
    if (!CanUndo())
        return;
    mCommands[--mCursor].undo();
    emit StateChanged();
}

void CommandHistory::Redo()
{
    if (!CanRedo())
        return;
    mCommands[mCursor++].redo();
    emit StateChanged();
}

void CommandHistory::Clear()
{
    mCommands.clear();
    mCursor = 0;
    mCleanCursor = 0;
    emit StateChanged();
}

void CommandHistory::SetClean()
{
    mCleanCursor = mCursor;
    emit StateChanged();
}

void CommandHistory::InvalidateClean()
{
    mCleanCursor = std::numeric_limits<size_t>::max();
    emit StateChanged();
}
