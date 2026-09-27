/**
 * @file EquipmentWorker.h
 * @brief Multithreaded simulation worker wrapping EquipmentState.
 *
 * Implements the QObject worker pattern, designed to be moved to a dedicated
 * QThread to execute simulation ticks without blocking the primary UI thread.
 */

#pragma once

#include <QObject>
#include <QTimer>
#include <memory>
#include "EquipmentState.h"

/**
 * @class EquipmentWorker
 * @brief Autonomous worker managing simulation time steps and command execution.
 */
class EquipmentWorker : public QObject {
    Q_OBJECT

public:
    static constexpr int TICK_INTERVAL_MS = 200;

    /**
     * @brief Constructs the EquipmentWorker.
     * @param parent Optional parent QObject.
     */
    explicit EquipmentWorker(QObject *parent = nullptr);

    /**
     * @brief Destructs worker and ensures timer teardown.
     */
    ~EquipmentWorker() override;

    /**
     * @brief Returns read-only access to current status.
     */
    [[nodiscard]] EquipmentStatus getStatus() const;

    /**
     * @brief Returns current position.
     */
    [[nodiscard]] double getPosition() const;

    /**
     * @brief Returns current temperature.
     */
    [[nodiscard]] double getTemperature() const;

public slots:
    /**
     * @brief Starts the simulation timer on the worker thread.
     */
    void start();

    /**
     * @brief Stops the simulation timer.
     */
    void stop();

    /**
     * @brief Requests transition to Running state.
     */
    void requestStart();

    /**
     * @brief Requests transition back to Idle state.
     */
    void requestStop();

    /**
     * @brief Requests stage homing sequence.
     */
    void requestHome();

    /**
     * @brief Requests recovery from Error state.
     */
    void requestReset();

    /**
     * @brief Simulates emergency stop / fault trigger.
     */
    void requestFault();

    /**
     * @brief Periodic tick invoked by QTimer.
     */
    void onTick();

signals:
    /**
     * @brief Emitted every tick with updated telemetry values.
     * @param position Current position in mm.
     * @param temperature Current chamber temperature in °C.
     * @param status Current operating status.
     */
    void dataUpdated(double position, double temperature, EquipmentStatus status);

    /**
     * @brief Emitted when operating status changes.
     * @param newStatus New equipment state.
     */
    void statusChanged(EquipmentStatus newStatus);

private:
    EquipmentState m_state;
    QTimer *m_timer{nullptr};
    quint64 m_tickCount{0};
};
