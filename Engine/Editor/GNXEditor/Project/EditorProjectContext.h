#pragma once

#include <QString>

struct EditorProjectContext
{
    QString engineContentRoot;
    QString projectFile;
    QString projectRoot;
    QString assetRoot;
    QString cacheRoot;
    QString settingsRoot;
    QString scenesRoot;
    QString sessionId;

    bool IsValid() const
    {
        return !engineContentRoot.isEmpty() && !projectRoot.isEmpty() && !assetRoot.isEmpty() &&
               !cacheRoot.isEmpty() && !sessionId.isEmpty();
    }
};
