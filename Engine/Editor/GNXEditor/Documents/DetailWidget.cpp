#include "DetailWidget.h"
#include "SceneDocument.h"
#include "SelectionService.h"
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>

DetailWidget::DetailWidget(SceneDocument &document, SelectionService &selection, QWidget *parent)
    : QWidget(parent), mDocument(document), mSelection(selection), mId(new QLabel(this)),
      mName(new QLineEdit(this))
{
    auto *layout = new QFormLayout(this);
    layout->addRow(tr("ID"), mId);
    layout->addRow(tr("名称"), mName);
    connect(&mSelection, &SelectionService::SelectionChanged, this,
            [this]
            {
                Refresh();
            });
    connect(&mDocument, &SceneDocument::Changed, this,
            [this]
            {
                Refresh();
            });
    connect(mName, &QLineEdit::editingFinished, this,
            [this]
            {
                if (!mRefreshing)
                    mDocument.RenameEntity(mSelection.SelectedId(), mName->text());
            });
    Refresh();
}

void DetailWidget::Refresh()
{
    mRefreshing = true;
    const SceneEntity *entity = mDocument.Find(mSelection.SelectedId());
    mId->setText(entity ? entity->id : QString());
    mName->setText(entity ? entity->name : QString());
    mName->setEnabled(entity != nullptr);
    mRefreshing = false;
}
