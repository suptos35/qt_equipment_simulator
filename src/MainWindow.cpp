/**
 * @file MainWindow.cpp
 * @brief Implementation of the primary application window.
 */

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Logging.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(std::make_unique<Ui::MainWindow>()) {
    ui->setupUi(this);

    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(ui->btnHome, &QPushButton::clicked, this, &MainWindow::onHomeClicked);
    connect(ui->btnReset, &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(ui->btnTriggerError, &QPushButton::clicked, this, &MainWindow::onTriggerErrorClicked);

    appendLogMessage(QStringLiteral("MainWindow initialized successfully."));
}

MainWindow::~MainWindow() = default;

void MainWindow::appendLogMessage(const QString &message) {
    ui->listEvents->addItem(message);
    ui->listEvents->scrollToBottom();
}

void MainWindow::onStartClicked() {
    qInfo(logGui) << "UI: Start button clicked.";
}

void MainWindow::onStopClicked() {
    qInfo(logGui) << "UI: Stop button clicked.";
}

void MainWindow::onHomeClicked() {
    qInfo(logGui) << "UI: Home button clicked.";
}

void MainWindow::onResetClicked() {
    qInfo(logGui) << "UI: Reset button clicked.";
}

void MainWindow::onTriggerErrorClicked() {
    qWarning(logGui) << "UI: Trigger Fault button clicked.";
}
