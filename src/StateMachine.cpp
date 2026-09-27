/**
 * @file StateMachine.cpp
 * @brief Implementation of equipment finite state machine transitions.
 */

#include "StateMachine.h"
#include "Logging.h"

StateMachine::StateMachine(QObject *parent)
    : QObject(parent),
      m_currentState(EquipmentStatus::Idle) {
}

EquipmentStatus StateMachine::getCurrentState() const noexcept {
    return m_currentState;
}

bool StateMachine::requestStart() {
    return attemptTransition(EquipmentStatus::Running, QStringLiteral("requestStart()"));
}

bool StateMachine::requestStop() {
    return attemptTransition(EquipmentStatus::Idle, QStringLiteral("requestStop()"));
}

bool StateMachine::requestHome() {
    return attemptTransition(EquipmentStatus::Homing, QStringLiteral("requestHome()"));
}

bool StateMachine::onHomeComplete() {
    return attemptTransition(EquipmentStatus::Idle, QStringLiteral("onHomeComplete()"));
}

bool StateMachine::requestReset() {
    return attemptTransition(EquipmentStatus::Idle, QStringLiteral("requestReset()"));
}

bool StateMachine::triggerFault(const QString &reason) {
    return attemptTransition(EquipmentStatus::Error, QString("triggerFault(%1)").arg(reason));
}

bool StateMachine::attemptTransition(EquipmentStatus target, const QString &trigger) {
    bool valid = false;

    // Fault can occur from any active operational state (Idle, Homing, Running)
    if (target == EquipmentStatus::Error) {
        if (m_currentState != EquipmentStatus::Error) {
            valid = true;
        } else {
            // Redundant fault when already in error
            valid = false;
        }
    }
    // Transition to Running is only permitted from Idle
    else if (target == EquipmentStatus::Running) {
        if (m_currentState == EquipmentStatus::Idle && trigger == QStringLiteral("requestStart()")) {
            valid = true;
        }
    }
    // Transition to Homing is only permitted from Idle
    else if (target == EquipmentStatus::Homing) {
        if (m_currentState == EquipmentStatus::Idle && trigger == QStringLiteral("requestHome()")) {
            valid = true;
        }
    }
    // Transition back to Idle is permitted from Homing (on complete), Running (on stop), or Error (on reset)
    else if (target == EquipmentStatus::Idle) {
        if (m_currentState == EquipmentStatus::Homing && trigger == QStringLiteral("onHomeComplete()")) {
            valid = true;
        } else if (m_currentState == EquipmentStatus::Running && trigger == QStringLiteral("requestStop()")) {
            valid = true;
        } else if (m_currentState == EquipmentStatus::Error && trigger == QStringLiteral("requestReset()")) {
            valid = true;
        }
    }

    if (valid) {
        EquipmentStatus prev = m_currentState;
        m_currentState = target;
        qInfo(logEquip) << "State transition ACCEPTED: ["
                        << EquipmentState::statusToString(prev) << "] -> ["
                        << EquipmentState::statusToString(target) << "] via" << trigger;
        emit stateChanged(prev, target);
        return true;
    }

    qWarning(logEquip) << "State transition REJECTED: cannot execute ["
                       << trigger << "] while in state ["
                       << EquipmentState::statusToString(m_currentState) << "]";
    emit transitionRejected(m_currentState, trigger);
    return false;
}
