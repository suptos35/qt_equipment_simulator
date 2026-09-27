/**
 * @file EquipmentWorker.cpp
 * @brief Implementation of multithreaded simulation worker.
 */

#include "EquipmentWorker.h"
#include "Logging.h"

#include <QThread>

EquipmentWorker::EquipmentWorker(QObject *parent)
    : QObject(parent),
      m_timer(new QTimer(this)) {
    connect(m_timer, &QTimer::timeout, this, &EquipmentWorker::onTick);
}

EquipmentWorker::~EquipmentWorker() {
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
}

EquipmentStatus EquipmentWorker::getStatus() const {
    return m_state.getStatus();
}

double EquipmentWorker::getPosition() const {
    return m_state.getPosition();
}

double EquipmentWorker::getTemperature() const {
    return m_state.getTemperature();
}

void EquipmentWorker::start() {
    qInfo(logThread) << "EquipmentWorker::start() active on thread:" << QThread::currentThreadId();
    if (!m_timer->isActive()) {
        m_timer->start(TICK_INTERVAL_MS);
    }
}

void EquipmentWorker::stop() {
    qInfo(logThread) << "EquipmentWorker::stop() on thread:" << QThread::currentThreadId();
    if (m_timer->isActive()) {
        m_timer->stop();
    }
}

void EquipmentWorker::requestStart() {
    if (m_state.getStatus() == EquipmentStatus::Idle) {
        m_state.setStatus(EquipmentStatus::Running);
        emit statusChanged(EquipmentStatus::Running);
    } else {
        qWarning(logEquip) << "Worker: Rejected requestStart() from status:"
                           << EquipmentState::statusToString(m_state.getStatus());
    }
}

void EquipmentWorker::requestStop() {
    if (m_state.getStatus() == EquipmentStatus::Running) {
        m_state.setStatus(EquipmentStatus::Idle);
        emit statusChanged(EquipmentStatus::Idle);
    } else {
        qWarning(logEquip) << "Worker: Rejected requestStop() from status:"
                           << EquipmentState::statusToString(m_state.getStatus());
    }
}

void EquipmentWorker::requestHome() {
    if (m_state.getStatus() == EquipmentStatus::Idle) {
        m_state.setStatus(EquipmentStatus::Homing);
        emit statusChanged(EquipmentStatus::Homing);
    } else {
        qWarning(logEquip) << "Worker: Rejected requestHome() from status:"
                           << EquipmentState::statusToString(m_state.getStatus());
    }
}

void EquipmentWorker::requestReset() {
    if (m_state.getStatus() == EquipmentStatus::Error) {
        m_state.setStatus(EquipmentStatus::Idle);
        emit statusChanged(EquipmentStatus::Idle);
    } else {
        qWarning(logEquip) << "Worker: Rejected requestReset() from non-error status:"
                           << EquipmentState::statusToString(m_state.getStatus());
    }
}

void EquipmentWorker::requestFault() {
    m_state.setStatus(EquipmentStatus::Error);
    emit statusChanged(EquipmentStatus::Error);
}

void EquipmentWorker::onTick() {
    EquipmentStatus prevStatus = m_state.getStatus();
    m_state.tick();
    EquipmentStatus currentStatus = m_state.getStatus();

    m_tickCount++;

    // Throttled debug logging: log every 10th tick to avoid log flooding
    if (m_tickCount % 10 == 0) {
        qDebug(logEquip) << "Worker tick #" << m_tickCount
                         << "Pos:" << m_state.getPosition()
                         << "Temp:" << m_state.getTemperature()
                         << "Status:" << EquipmentState::statusToString(currentStatus);
    }

    // Always emit telemetry to observers across thread boundary
    emit dataUpdated(m_state.getPosition(), m_state.getTemperature(), currentStatus);

    if (prevStatus != currentStatus) {
        emit statusChanged(currentStatus);
    }
}
