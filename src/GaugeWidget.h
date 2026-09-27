/**
 * @file GaugeWidget.h
 * @brief Custom QPainter-rendered analog gauge widget for telemetry visualization.
 */

#pragma once

#include <QWidget>
#include <QString>

/**
 * @class GaugeWidget
 * @brief Visualizes telemetry readings with an analog dial, dynamic gradient, and needle.
 */
class GaugeWidget : public QWidget {
    Q_OBJECT

public:
    explicit GaugeWidget(QWidget *parent = nullptr);

    void setValue(double value);
    void setRange(double minVal, double maxVal);
    void setUnit(const QString &unit);
    void setTitle(const QString &title);

    [[nodiscard]] double getValue() const noexcept { return m_value; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_value{25.0};
    double m_minValue{0.0};
    double m_maxValue{100.0};
    QString m_unit{QStringLiteral("°C")};
    QString m_title{QStringLiteral("Temperature")};
};
