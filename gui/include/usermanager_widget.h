#ifndef USERMANAGER_WIDGET_H
#define USERMANAGER_WIDGET_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class UserManagerWidget : public QDialog {
    Q_OBJECT
public:
    explicit UserManagerWidget(QWidget *parent = nullptr);
    
public slots:
    void refreshList();
    void deleteSelectedUser();

private:
    QTableWidget *m_table;
    QPushButton *m_btnRefresh;
    QPushButton *m_btnDelete;
    QPushButton *m_btnClose;
};

#endif // USERMANAGER_WIDGET_H
