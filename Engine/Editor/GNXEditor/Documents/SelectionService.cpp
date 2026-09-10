#include "SelectionService.h"

SelectionService::SelectionService(QObject *parent) : QObject(parent)
{
}

void SelectionService::Select(const QString &id)
{
    if (mSelectedId == id)
        return;
    mSelectedId = id;
    emit SelectionChanged(mSelectedId);
}

void SelectionService::Clear()
{
    Select({});
}
