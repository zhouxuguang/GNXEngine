#include "HierarchyWidget.h"
#include "SceneDocument.h"
#include "SelectionService.h"
#include <QAction>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QVBoxLayout>

HierarchyWidget::HierarchyWidget(SceneDocument &document, SelectionService &selection,
                                 QWidget *parent)
    : QWidget(parent), mDocument(document), mSelection(selection), mList(new QListWidget(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mList);
    mList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&mDocument, &SceneDocument::Changed, this, &HierarchyWidget::Refresh);
    connect(mList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *item)
            {
                item ? mSelection.Select(item->data(Qt::UserRole).toString()) : mSelection.Clear();
            });
    connect(mList, &QListWidget::customContextMenuRequested, this,
            [this](const QPoint &point)
            {
                QMenu menu(this);
                menu.addAction(tr("添加实体"), this,
                               [this]
                               {
                                   bool accepted = false;
                                   const QString name = QInputDialog::getText(
                                       this, tr("添加实体"), tr("名称"), QLineEdit::Normal,
                                       tr("Entity"), &accepted);
                                   if (accepted)
                                       mDocument.AddEntity(name);
                               });
                if (QListWidgetItem *item = mList->itemAt(point))
                    menu.addAction(tr("删除"), this,
                                   [this, item]
                                   {
                                       mDocument.RemoveEntity(item->data(Qt::UserRole).toString());
                                   });
                menu.exec(mList->viewport()->mapToGlobal(point));
            });
    Refresh();
}

void HierarchyWidget::Refresh()
{
    const QString selected = mSelection.SelectedId();
    mList->clear();
    for (const SceneEntity &entity : mDocument.Entities())
    {
        auto *item = new QListWidgetItem(entity.name, mList);
        item->setData(Qt::UserRole, entity.id);
        if (entity.id == selected)
            mList->setCurrentItem(item);
    }
}
