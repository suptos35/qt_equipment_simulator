/**
 * @file StateMachine.h
 * @brief Formal finite state machine governing equipment operational lifecycle.
 *
 * Enforces valid state transitions according to semiconductor equipment automation rules:
 * - Idle -> Homing -> Running -> Idle
 * - Error reachable from ANY state (Idle, Homing, Running)
 * - Error exitable ONLY via requestReset() returning to Idle
 * - Strict rejection and logging for all invalid transitions
 */

#pragma once

#include <QObject>
#include <QString>
#include "EquipmentState.h"

/**
 * @class StateMachine
 * @brief Manages machine state lifecycle and enforces valid operational transitions.
 */
class StateMachine : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructs StateMachine initialized in Idle state.
     * @param parent Optional parent QObject.
     */
    explicit StateMachine(QObject *parent = nullptr);

    /**
     * @brief Retrieves current state.
     */
    [[nodiscard]] EquipmentStatus getCurrentState() const noexcept;

    /**
     * @brief Attempts transition from Idle to Running.
     * @return true if accepted, false if rejected.
     */
    bool requestStart();

    /**
     * @brief Attempts transition from Running to Idle.
     * @return true if accepted, false if rejected.
     */
    bool requestStop();

    /**
     * @brief Attempts transition from Idle to Homing.
     * @return true if accepted, false if rejected.
     */
    bool requestHome();

    /**
     * @brief Notifies state machine that homing routine completed (Homing -> Idle).
     * @return true if accepted, false if rejected.
     */
    bool onHomeComplete();

    /**
     * @brief Attempts recovery from Error to Idle.
     * @return true if accepted, false if rejected.
     */
    bool requestReset();

    /**
     * @brief Triggers an emergency fault/stop from any state to Error.
     * @param reason Diagnostic explanation for fault.
     * @return true if accepted.
     */
    bool triggerFault(const QString &reason = QStringLiteral("Fault Triggered"));

signals:
    /**
     * @brief Emitted when a valid state transition occurs.
     * @param from Previous state.
     * @param to New state.
     */
    void stateChanged(EquipmentStatus from, EquipmentStatus to);

    /**
     * @brief Emitted when an invalid transition attempt is rejected.
     * @param current Current machine state.
     * @param trigger Name of rejected action.
     */
    void transitionRejected(EquipmentStatus current, const QString &trigger);

private:
    bool attemptTransition(EquipmentStatus target, const QString &trigger);

    EquipmentStatus m_currentState{EquipmentStatus::Idle};
};
