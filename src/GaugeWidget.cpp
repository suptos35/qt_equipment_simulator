/**
 * @file GaugeWidget.cpp
 * @brief Implementation of custom QPainter telemetry gauge.
 */

#include "GaugeWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QConicalGradient>
#include <cmath>
#include <algorithm>

GaugeWidget::GaugeWidget(QWidget *parent)
    : QWidget(parent) {
    setMinimumSize(180, 180);
}

void GaugeWidget::setValue(double value) {
    if (std::abs(m_value - value) > 0.01) {
        m_value = value;
        update();
    }
}

void GaugeWidget::setRange(double minVal, double maxVal) {
    m_minValue = minVal;
    m_maxValue = (maxVal > minVal) ? maxVal : (minVal + 1.0);
    update();
}

void GaugeWidget::setUnit(const QString &unit) {
    m_unit = unit;
    update();
}

void GaugeWidget::setTitle(const QString &title) {
    m_title = title;
    update();
}

void GaugeWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int side = std::min(width(), height());
    painter.setViewport((width() - side) / 2, (height() - side) / 2, side, side);
    painter.setWindow(-100, -100, 200, 200);

    // 1. Draw outer gauge bezel and background
    QRadialGradient bgGrad(0, 0, 95);
    bgGrad.setColorAt(0.0, QColor(35, 38, 45));
    bgGrad.setColorAt(0.9, QColor(25, 28, 34));
    bgGrad.setColorAt(1.0, QColor(15, 18, 22));
    painter.setBrush(bgGrad);
    painter.setPen(QPen(QColor(60, 68, 80), 3));
    painter.drawEllipse(-90, -90, 180, 180);

    // 2. Draw gauge arc track (240 degrees span from 150 deg to 390 deg)
    const int startAngle = 150 * 16;
    const int spanAngle = -240 * 16;

    QRectF arcRect(-70, -70, 140, 140);
    painter.setPen(QPen(QColor(50, 55, 65), 10, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(arcRect, startAngle, spanAngle);

    // 3. Draw active colored arc
    double ratio = (std::clamp(m_value, m_minValue, m_maxValue) - m_minValue) / (m_maxValue - m_minValue);
    int activeSpan = static_cast<int>(-240.0 * ratio * 16.0);

    QColor activeColor;
    if (ratio < 0.6) {
        activeColor = QColor(46, 204, 113); // Normal green
    } else if (ratio < 0.85) {
        activeColor = QColor(241, 196, 15); // Warning amber
    } else {
        activeColor = QColor(231, 76, 60);  // High critical red
    }

    painter.setPen(QPen(activeColor, 10, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(arcRect, startAngle, activeSpan);

    // 4. Center readout texts
    painter.setPen(QColor(220, 225, 235));
    QFont titleFont = font();
    titleFont.setPointSize(8);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(QRectF(-80, -35, 160, 20), Qt::AlignCenter, m_title);

    QFont valueFont = font();
    valueFont.setPointSize(16);
    valueFont.setBold(true);
    painter.setFont(valueFont);
    QString valueText = QString("%1 %2").arg(m_value, 0, 'f', 1).arg(m_unit);
    painter.drawText(QRectF(-80, -10, 160, 30), Qt::AlignCenter, valueText);

    // 5. Min / Max scale labels
    QFont scaleFont = font();
    scaleFont.setPointSize(7);
    painter.setFont(scaleFont);
    painter.setPen(QColor(140, 150, 165));
    painter.drawText(QRectF(-75, 45, 40, 18), Qt::AlignCenter, QString::number(static_cast<int>(m_minValue)));
    painter.drawText(QRectF(35, 45, 40, 18), Qt::AlignCenter, QString::number(static_cast<int>(m_maxValue)));
}
