#pragma once

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFont>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSize>
#include <QSizePolicy>
#include <QSlider>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "SettingsPages.h"

class GraphicsPage : public SettingPage
{
public:
    GraphicsPage();
    void LoadConfig();
    void SaveConfig();
private:
    void BuildOptions();

    QVBoxLayout* mainLayout;
};
