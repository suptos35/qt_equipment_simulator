/**
 * @file MainWindow.h
 * @brief Primary user interface window for the Qt Equipment Simulator.
 */

#pragma once

#include <QMainWindow>
#include <QThread>
#include <QListWidget>
#include <memory>

#include "EquipmentWorker.h"
#include "StateMachine.h"
#include "GaugeWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @class MainWindow
 * @brief Coordinates GUI events, multithreaded simulation telemetry, and state machine controls.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    /**
     * @brief Accessor to the internal event log widget for automated testing.
     */
    [[nodiscard]] QListWidget* getLogListWidget() const;

    /**
     * @brief Accessor to the state machine for test inspection.
     */
    [[nodiscard]] StateMachine& getStateMachine() noexcept;

public slots:
    /**
     * @brief Thread-safe slot invoked via Qt::QueuedConnection by the global logging handler.
     * @param msgType QtMsgType enum value.
     * @param formattedMsg Full formatted diagnostic string.
     */
    Q_INVOKABLE void appendLogEntry(int msgType, const QString &formattedMsg);

    void onDataUpdated(double position, double temperature, EquipmentStatus status);
    void onWorkerStatusChanged(EquipmentStatus newStatus);
    void onStateChanged(EquipmentStatus from, EquipmentStatus to);
    void onTransitionRejected(EquipmentStatus current, const QString &trigger);

private slots:
    void onStartClicked();
    void onStopClicked();
    void onHomeClicked();
    void onResetClicked();
    void onTriggerErrorClicked();

private:
    void setupConnections();
    void updateStatusDisplay(EquipmentStatus status);

    std::unique_ptr<Ui::MainWindow> ui;
    QThread *m_workerThread{nullptr};
    EquipmentWorker *m_worker{nullptr};
    StateMachine m_stateMachine;
};
