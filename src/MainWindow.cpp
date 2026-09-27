/**
 * @file MainWindow.cpp
 * @brief Implementation of application main window, wiring UI, worker, and state machine.
 */

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Logging.h"

#include <QColor>
#include <QListWidgetItem>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(std::make_unique<Ui::MainWindow>()) {
    ui->setupUi(this);

    qRegisterMetaType<EquipmentStatus>("EquipmentStatus");

    // Configure analog gauge
    ui->gaugeTemperature->setRange(0.0, 100.0);
    ui->gaugeTemperature->setUnit(QStringLiteral("°C"));
    ui->gaugeTemperature->setTitle(QStringLiteral("Chamber Temp"));

    // Register this window to receive thread-safe log messages
    Logging::registerGuiReceiver(this, "appendLogEntry");

    setupConnections();
    updateStatusDisplay(EquipmentStatus::Idle);

    qInfo(logGui) << "MainWindow GUI initialized and operational.";
}

MainWindow::~MainWindow() {
    Logging::unregisterGuiReceiver();

    if (m_workerThread) {
        if (m_worker) {
            QMetaObject::invokeMethod(m_worker, "stop", Qt::BlockingQueuedConnection);
        }
        m_workerThread->quit();
        m_workerThread->wait(2000);
    }
}

QListWidget* MainWindow::getLogListWidget() const {
    return ui->listEvents;
}

StateMachine& MainWindow::getStateMachine() noexcept {
    return m_stateMachine;
}

void MainWindow::setupConnections() {
    // 1. UI Button triggers
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(ui->btnHome, &QPushButton::clicked, this, &MainWindow::onHomeClicked);
    connect(ui->btnReset, &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(ui->btnTriggerError, &QPushButton::clicked, this, &MainWindow::onTriggerErrorClicked);

    // 2. StateMachine notifications
    connect(&m_stateMachine, &StateMachine::stateChanged, this, &MainWindow::onStateChanged);
    connect(&m_stateMachine, &StateMachine::transitionRejected, this, &MainWindow::onTransitionRejected);

    // 3. Worker Thread allocation and cross-thread connections
    m_worker = new EquipmentWorker(); // No parent: required before moveToThread
    m_workerThread = new QThread(this);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &EquipmentWorker::start);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    connect(m_worker, &EquipmentWorker::dataUpdated, this, &MainWindow::onDataUpdated, Qt::QueuedConnection);
    connect(m_worker, &EquipmentWorker::statusChanged, this, &MainWindow::onWorkerStatusChanged, Qt::QueuedConnection);

    m_workerThread->start();
}

void MainWindow::updateStatusDisplay(EquipmentStatus status) {
    ui->lblStatus->setText(EquipmentState::statusToString(status));

    switch (status) {
    case EquipmentStatus::Idle:
        ui->lblStatus->setStyleSheet(QStringLiteral("color: #2980b9; padding: 2px 8px; background: #eaf2f8; border-radius: 4px;"));
        ui->btnStart->setEnabled(true);
        ui->btnHome->setEnabled(true);
        ui->btnStop->setEnabled(false);
        ui->btnReset->setEnabled(false);
        break;
    case EquipmentStatus::Homing:
        ui->lblStatus->setStyleSheet(QStringLiteral("color: #8e44ad; padding: 2px 8px; background: #f4ecf7; border-radius: 4px;"));
        ui->btnStart->setEnabled(false);
        ui->btnHome->setEnabled(false);
        ui->btnStop->setEnabled(false);
        ui->btnReset->setEnabled(false);
        break;
    case EquipmentStatus::Running:
        ui->lblStatus->setStyleSheet(QStringLiteral("color: #27ae60; padding: 2px 8px; background: #eafaf1; border-radius: 4px;"));
        ui->btnStart->setEnabled(false);
        ui->btnHome->setEnabled(false);
        ui->btnStop->setEnabled(true);
        ui->btnReset->setEnabled(false);
        break;
    case EquipmentStatus::Error:
        ui->lblStatus->setStyleSheet(QStringLiteral("color: #c0392b; padding: 2px 8px; background: #fadbd8; border-radius: 4px; font-weight: bold;"));
        ui->btnStart->setEnabled(false);
        ui->btnHome->setEnabled(false);
        ui->btnStop->setEnabled(false);
        ui->btnReset->setEnabled(true);
        break;
    }
}

void MainWindow::onDataUpdated(double position, double temperature, EquipmentStatus status) {
    ui->lblPosition->setText(QString("%1 mm").arg(position, 0, 'f', 2));
    ui->lblTemperature->setText(QString("%1 °C").arg(temperature, 0, 'f', 2));
    ui->gaugeTemperature->setValue(temperature);

    // Keep UI status indicator synchronized
    updateStatusDisplay(status);
}

void MainWindow::onWorkerStatusChanged(EquipmentStatus newStatus) {
    if (newStatus == EquipmentStatus::Idle && m_stateMachine.getCurrentState() == EquipmentStatus::Homing) {
        // Homing routine finished physical motion on worker
        m_stateMachine.onHomeComplete();
    }
}

void MainWindow::onStateChanged(EquipmentStatus from, EquipmentStatus to) {
    Q_UNUSED(from);
    updateStatusDisplay(to);
}

void MainWindow::onTransitionRejected(EquipmentStatus current, const QString &trigger) {
    Q_UNUSED(current);
    Q_UNUSED(trigger);
    statusBar()->showMessage(QString("Transition rejected: %1").arg(trigger), 3000);
}

void MainWindow::onStartClicked() {
    if (m_stateMachine.requestStart()) {
        QMetaObject::invokeMethod(m_worker, "requestStart", Qt::QueuedConnection);
    }
}

void MainWindow::onStopClicked() {
    if (m_stateMachine.requestStop()) {
        QMetaObject::invokeMethod(m_worker, "requestStop", Qt::QueuedConnection);
    }
}

void MainWindow::onHomeClicked() {
    if (m_stateMachine.requestHome()) {
        QMetaObject::invokeMethod(m_worker, "requestHome", Qt::QueuedConnection);
    }
}

void MainWindow::onResetClicked() {
    if (m_stateMachine.requestReset()) {
        QMetaObject::invokeMethod(m_worker, "requestReset", Qt::QueuedConnection);
    }
}

void MainWindow::onTriggerErrorClicked() {
    if (m_stateMachine.triggerFault(QStringLiteral("Manual Fault Button"))) {
        QMetaObject::invokeMethod(m_worker, "requestFault", Qt::QueuedConnection);
    }
}

void MainWindow::appendLogEntry(int msgType, const QString &formattedMsg) {
    auto *item = new QListWidgetItem(formattedMsg);

    // Color-code based on severity
    switch (msgType) {
    case QtDebugMsg:
        item->setForeground(QColor(120, 130, 140));
        break;
    case QtInfoMsg:
        item->setForeground(QColor(40, 116, 166));
        break;
    case QtWarningMsg:
        item->setForeground(QColor(185, 119, 14));
        item->setFont(QFont(item->font().family(), item->font().pointSize(), QFont::Bold));
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        item->setForeground(QColor(192, 57, 43));
        item->setFont(QFont(item->font().family(), item->font().pointSize(), QFont::Bold));
        break;
    }

    ui->listEvents->addItem(item);

    // Bound on-screen log memory: keep last 500 items
    if (ui->listEvents->count() > 500) {
        delete ui->listEvents->takeItem(0);
    }

    ui->listEvents->scrollToBottom();
}
