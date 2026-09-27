/**
 * @file EquipmentState.cpp
 * @brief Implementation of pure equipment simulation physics and state logic.
 */

#include "EquipmentState.h"
#include "Logging.h"

#include <algorithm>
#include <cmath>

EquipmentState::EquipmentState() = default;

double EquipmentState::pseudoRandomDelta() {
    // Simple LCG for deterministic, platform-independent, lightweight pseudo-random numbers
    m_rngSeed = (m_rngSeed * 1103515245 + 12345) & 0x7fffffff;
    // Returns value roughly in range [-0.05, 0.05]
    return (static_cast<double>(m_rngSeed % 1000) / 10000.0) - 0.05;
}

void EquipmentState::tick() {
    switch (m_status) {
    case EquipmentStatus::Homing: {
        // Move towards home reference (0.0 mm)
        if (m_position > 0.0) {
            m_position -= 1.0;
            if (m_position <= 0.0) {
                m_position = 0.0;
                m_status = EquipmentStatus::Idle;
                qInfo(logEquip) << "Homing routine completed successfully. Stage at 0.00 mm. State -> Idle";
            }
        } else {
            m_position = 0.0;
            m_status = EquipmentStatus::Idle;
            qInfo(logEquip) << "Homing completed immediately (already at home position). State -> Idle";
        }
        break;
    }

    case EquipmentStatus::Running: {
        // Translate stage
        m_position += m_velocity + (pseudoRandomDelta() * 0.1);
        if (m_position >= MAX_POSITION) {
            m_position = MAX_POSITION;
            m_velocity = -std::abs(m_velocity);
        } else if (m_position <= MIN_POSITION) {
            m_position = MIN_POSITION;
            m_velocity = std::abs(m_velocity);
        }

        // Temperature warms toward operating point
        double tempDelta = (OPERATING_TEMP_TARGET - m_temperature) * 0.05 + pseudoRandomDelta();
        m_temperature += tempDelta;
        break;
    }

    case EquipmentStatus::Idle:
    case EquipmentStatus::Error: {
        // Small ambient fluctuation in position
        m_position += pseudoRandomDelta() * 0.01;
        m_position = std::clamp(m_position, MIN_POSITION, MAX_POSITION);

        // Chamber cools toward ambient
        double tempDelta = (AMBIENT_TEMP - m_temperature) * 0.02 + (pseudoRandomDelta() * 0.05);
        m_temperature += tempDelta;
        break;
    }
    }

    // Clamp physics values within physical bounds
    m_position = std::clamp(m_position, MIN_POSITION, MAX_POSITION);
    m_temperature = std::clamp(m_temperature, 15.0, MAX_SAFE_TEMP);
}

void EquipmentState::setStatus(EquipmentStatus newStatus) {
    if (m_status != newStatus) {
        qInfo(logEquip) << "EquipmentState status changed from"
                        << statusToString(m_status) << "to" << statusToString(newStatus);
        m_status = newStatus;
    }
}

EquipmentStatus EquipmentState::getStatus() const noexcept {
    return m_status;
}

double EquipmentState::getPosition() const noexcept {
    return m_position;
}

double EquipmentState::getTemperature() const noexcept {
    return m_temperature;
}

void EquipmentState::setPosition(double pos) noexcept {
    m_position = std::clamp(pos, MIN_POSITION, MAX_POSITION);
}

const char* EquipmentState::statusToString(EquipmentStatus status) noexcept {
    switch (status) {
    case EquipmentStatus::Idle:    return "Idle";
    case EquipmentStatus::Homing:  return "Homing";
    case EquipmentStatus::Running: return "Running";
    case EquipmentStatus::Error:   return "Error";
    }
    return "Unknown";
}
