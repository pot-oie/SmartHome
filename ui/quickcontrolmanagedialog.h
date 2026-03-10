#pragma once
#include <QDialog>
#include <QListWidget>
#include "services/quickcontrolservice.h"

class QuickControlManageDialog : public QDialog
{
    Q_OBJECT
public:
    explicit QuickControlManageDialog(QWidget *parent = nullptr);

private slots:
    void onSaveClicked();

private:
    QListWidget *m_listWidget;
    QuickControlService m_service;
    QList<QuickControlManageItem> m_items;
};