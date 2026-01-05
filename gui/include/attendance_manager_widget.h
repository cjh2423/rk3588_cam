#ifndef ATTENDANCEMANAGER_WIDGET_H
#define ATTENDANCEMANAGER_WIDGET_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFutureWatcher>
#include "database/attendance_dao.h" // For AttendanceJoinedRecord

class AttendanceManagerWidget : public QDialog {
    Q_OBJECT
public:
    explicit AttendanceManagerWidget(QWidget *parent = nullptr);
    
public slots:
    void refreshList();
    void deleteSelectedRecord();
    void exportRecords();
    
private slots:
    void onRefreshFinished();

private:
    QLineEdit *m_searchEdit;
    QTableWidget *m_table;
    QPushButton *m_btnRefresh;
    QPushButton *m_btnDelete;
    QPushButton *m_btnExport;
    QPushButton *m_btnClose;
    
    // Async
    QFutureWatcher<std::vector<db::AttendanceJoinedRecord>> m_watcher;
};

#endif // ATTENDANCEMANAGER_WIDGET_H
