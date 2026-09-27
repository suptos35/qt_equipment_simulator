/**
 * @file MainWindow.h
 * @brief Primary user interface window for the Qt Equipment Simulator.
 */

#pragma once

#include <QMainWindow>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @class MainWindow
 * @brief Manages the main operator interface, telemetry displays, and control triggers.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Constructs the MainWindow.
     * @param parent Optional parent QWidget.
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Destructs the MainWindow.
     */
    ~MainWindow() override;

public slots:
    /**
     * @brief Appends a diagnostic or status message to the UI event log.
     * @param message Text string to append.
     */
    void appendLogMessage(const QString &message);

private slots:
    void onStartClicked();
    void onStopClicked();
    void onHomeClicked();
    void onResetClicked();
    void onTriggerErrorClicked();

private:
    std::unique_ptr<Ui::MainWindow> ui;
};
