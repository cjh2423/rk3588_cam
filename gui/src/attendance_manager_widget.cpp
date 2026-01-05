#include "attendance_manager_widget.h"
#include "database/attendance_dao.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QtConcurrent>         // 用于异步操作

AttendanceManagerWidget::AttendanceManagerWidget(QWidget *parent) : QDialog(parent) {
    setWindowTitle("考勤管理");
    resize(900, 600);
    
    auto *mainLayout = new QVBoxLayout(this);
    
    // Top Bar: Search
    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("搜索 (姓名/部门/ID):", this));
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("输入关键字...");
    topLayout->addWidget(m_searchEdit);
    mainLayout->addLayout(topLayout);
    
    // Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"记录ID", "姓名", "部门", "类型", "时间", "状态", "相似度"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents); // 确保时间列不被遮挡
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_table);
    
    // Buttons
    auto *btnLayout = new QHBoxLayout();
    m_btnRefresh = new QPushButton("刷新", this);
    m_btnDelete = new QPushButton("删除记录", this);
    m_btnExport = new QPushButton("导出记录", this);
    m_btnClose = new QPushButton("关闭", this);
    
    btnLayout->addWidget(m_btnRefresh);
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addWidget(m_btnExport);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnClose);
    mainLayout->addLayout(btnLayout);
    
    // Connects
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AttendanceManagerWidget::refreshList);
    connect(m_btnRefresh, &QPushButton::clicked, this, &AttendanceManagerWidget::refreshList);
    connect(m_btnDelete, &QPushButton::clicked, this, &AttendanceManagerWidget::deleteSelectedRecord);
    connect(m_btnExport, &QPushButton::clicked, this, &AttendanceManagerWidget::exportRecords);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);
    
    // Async Watcher 用于刷新列表
    connect(&m_watcher, &QFutureWatcher<std::vector<db::AttendanceJoinedRecord>>::finished, 
            this, &AttendanceManagerWidget::onRefreshFinished);
    
    refreshList();
}

void AttendanceManagerWidget::refreshList() {
    m_btnRefresh->setEnabled(false);
    m_table->setDisabled(true); // Prevent interaction while loading
    
    std::string keyword = m_searchEdit->text().toStdString();
    // 异步查询数据库 : 直接把一个函数扔到后台线程去跑
    QFuture<std::vector<db::AttendanceJoinedRecord>> future = QtConcurrent::run([keyword](){
        db::AttendanceDao dao;
        return dao.search_records_joined(keyword);
    });
    
    m_watcher.setFuture(future);
}

void AttendanceManagerWidget::onRefreshFinished() {
    auto records = m_watcher.result();
    
    m_table->setRowCount(0);
    for (const auto& r : records) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(r.record_id)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(r.user_name)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(r.department)));
        
        QString typeStr = (r.check_type == 1) ? "签到" : "签退";
        m_table->setItem(row, 3, new QTableWidgetItem(typeStr));
        
        QString timeStr = QDateTime::fromTime_t(r.check_time).toString("yyyy-MM-dd HH:mm:ss");
        m_table->setItem(row, 4, new QTableWidgetItem(timeStr));
        
        QString statusStr;
        if (r.status == 1) statusStr = "正常";
        else if (r.status == 2) statusStr = "迟到";
        else if (r.status == 3) statusStr = "早退";
        else statusStr = "未知";
        
        // 状态加颜色
        QTableWidgetItem* statusItem = new QTableWidgetItem(statusStr);
        if (r.status != 1) {
            statusItem->setForeground(Qt::red);
        }
        m_table->setItem(row, 5, statusItem);
        
        m_table->setItem(row, 6, new QTableWidgetItem(QString::number(r.similarity, 'f', 2)));
    }
    
    m_btnRefresh->setEnabled(true);
    m_table->setDisabled(false);
}

void AttendanceManagerWidget::deleteSelectedRecord() {
    auto items = m_table->selectedItems();
    if (items.isEmpty()) return;
    
    int row = items.first()->row();
    int64_t id = m_table->item(row, 0)->text().toLongLong();
    
    if (QMessageBox::question(this, "确认删除", 
            "确定要删除这条考勤记录吗？") == QMessageBox::Yes) {
        db::AttendanceDao dao;
        if (dao.delete_record(id)) {
            refreshList();
        } else {
            QMessageBox::warning(this, "错误", "删除失败！");
        }
    }
}

void AttendanceManagerWidget::exportRecords() {
    QString fileName = QFileDialog::getSaveFileName(this, "导出考勤记录", 
                                                    "attendance_report.csv", 
                                                    "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法打开文件进行写入！");
        return;
    }
    
    QTextStream out(&file);
    // BOM for Excel
    out << "\xEF\xBB\xBF";
    out << "Record ID,Name,Department,Type,Time,Status,Similarity\n";
    
    int rows = m_table->rowCount();
    for (int i = 0; i < rows; ++i) {
        QStringList rowData;
        for (int j = 0; j < 7; ++j) {
            rowData << m_table->item(i, j)->text();
        }
        out << rowData.join(",") << "\n";
    }
    
    file.close();
    QMessageBox::information(this, "成功", "导出完成！");
}
