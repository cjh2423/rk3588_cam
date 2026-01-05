#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QSlider>
#include <QComboBox> // Changed from QTimeEdit
#include <QCheckBox>
#include <QPushButton>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void saveSettings();

private:
    void loadCurrentSettings();

    // Controls
    QDoubleSpinBox *m_spinThreshold;
    QSpinBox *m_spinSampleCount;
    
    // Time Selection via ComboBoxes
    QComboBox *m_comboStartHour;
    QComboBox *m_comboStartMin;
    QComboBox *m_comboEndHour;
    QComboBox *m_comboEndMin;
    
    QSpinBox *m_spinLateThreshold;
    QSpinBox *m_spinEarlyThreshold;
    
    QSpinBox *m_spinVolume;
    QCheckBox *m_chkShowFps;
    
    QPushButton *m_btnSave;
    QPushButton *m_btnCancel;
};

#endif // SETTINGS_DIALOG_H
