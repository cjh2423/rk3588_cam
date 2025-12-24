#include "usermanager_widget.h"
#include "database/user_dao.h"
#include "database/face_feature_dao.h"
#include "service/feature_library.h"
#include <QHeaderView>
#include <QMessageBox>

UserManagerWidget::UserManagerWidget(QWidget *parent) : QDialog(parent) {
    setWindowTitle("用户管理");
    resize(600, 400);
    
    auto *mainLayout = new QVBoxLayout(this);
    
    // Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"用户ID", "姓名", "工号", "部门"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_table);
    
    // Buttons
    auto *btnLayout = new QHBoxLayout();
    m_btnRefresh = new QPushButton("刷新列表", this);
    m_btnDelete = new QPushButton("删除用户", this);
    m_btnClose = new QPushButton("关闭", this);
    
    btnLayout->addWidget(m_btnRefresh);
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnClose);
    mainLayout->addLayout(btnLayout);
    
    connect(m_btnRefresh, &QPushButton::clicked, this, &UserManagerWidget::refreshList);
    connect(m_btnDelete, &QPushButton::clicked, this, &UserManagerWidget::deleteSelectedUser);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);
    
    refreshList();
}

void UserManagerWidget::refreshList() {
    db::UserDao dao;
    auto users = dao.get_all_active_users();
    
    m_table->setRowCount(0);
    for (const auto& u : users) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(u.user_id)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(u.user_name)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(u.employee_id)));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(u.department)));
    }
}

void UserManagerWidget::deleteSelectedUser() {
    auto items = m_table->selectedItems();
    if (items.isEmpty()) return;
    
    int row = items.first()->row();
    int64_t userId = m_table->item(row, 0)->text().toLongLong();
    QString userName = m_table->item(row, 1)->text();
    
    if (QMessageBox::question(this, "确认删除", 
            "确定要删除用户: " + userName + " 吗？") == QMessageBox::Yes) {
        
        db::UserDao userDao;
        db::FaceFeatureDao featureDao;
        
        // Remove features first
        featureDao.delete_features_by_user_id(userId);
        // Remove user (soft delete usually, but here hard delete for simplicity)
        userDao.delete_user(userId);
        
        // Reload library
        service::FeatureLibrary::instance().load_from_database();
        
        refreshList();
    }
}
