/**
 * @file EquipmentState.h
 * @brief Pure C++ model for automated equipment simulation.
 *
 * Implements equipment telemetry, status management, and physics simulation
 * updates without direct coupling to Qt GUI or threading components.
 */

#pragma once

#include <string>

/**
 * @enum EquipmentStatus
 * @brief Discrete operational states of the semiconductor equipment.
 */
enum class EquipmentStatus {
    Idle,    ///< Ready and waiting for commands
    Homing,  ///< Calibrating stage position to home reference (0.0 mm)
    Running, ///< Actively processing wafers / operating stage
    Error    ///< Fault triggered; safety stop engaged
};

/**
 * @class EquipmentState
 * @brief Encapsulates position, chamber temperature, and lifecycle state.
 */
class EquipmentState {
public:
    static constexpr double MIN_POSITION = 0.0;
    static constexpr double MAX_POSITION = 100.0;
    static constexpr double AMBIENT_TEMP = 25.0;
    static constexpr double OPERATING_TEMP_TARGET = 50.0;
    static constexpr double MAX_SAFE_TEMP = 85.0;

    /**
     * @brief Constructs an EquipmentState in Idle mode.
     */
    EquipmentState();

    /**
     * @brief Advances the internal physics simulation by one discrete time tick.
     */
    void tick();

    /**
     * @brief Updates the equipment operating state.
     * @param newStatus Target status.
     */
    void setStatus(EquipmentStatus newStatus);

    /**
     * @brief Retrieves current operating state.
     * @return Current EquipmentStatus.
     */
    [[nodiscard]] EquipmentStatus getStatus() const noexcept;

    /**
     * @brief Retrieves stage position in millimeters.
     * @return Position (mm).
     */
    [[nodiscard]] double getPosition() const noexcept;

    /**
     * @brief Retrieves chamber temperature in degrees Celsius.
     * @return Temperature (°C).
     */
    [[nodiscard]] double getTemperature() const noexcept;

    /**
     * @brief Forces position to a specific value (useful for calibration/testing).
     * @param pos New position.
     */
    void setPosition(double pos) noexcept;

    /**
     * @brief Helper string representation for status enums.
     * @param status Equipment status.
     * @return String literal representation.
     */
    static const char* statusToString(EquipmentStatus status) noexcept;

private:
    double m_position{0.0};
    double m_temperature{AMBIENT_TEMP};
    EquipmentStatus m_status{EquipmentStatus::Idle};
    double m_velocity{1.0};
    unsigned int m_rngSeed{12345};

    double pseudoRandomDelta();
};
