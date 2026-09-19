#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QMap>
#include <QWidget>
#include <QVector>

struct Patient {
    int id;
    int heart_rate;
    int spo2;
    int rr;
    bool is_critical;
    QString blood_pressure;
    double amplitude;
    QString wave_type;
    double current_ecg;
    qint64 cycle_start;
    double current_interval;
    double sys;
    double dia;
};

// Custom widget to draw the live waveform
class GraphWidget : public QWidget {
    Q_OBJECT
public:
    explicit GraphWidget(QWidget *parent = nullptr);
    void addPoint(double val);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QVector<double> history;
    const int maxPoints = 60;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void toggleTcp();
    void toggleUdp();
    void processTcpTick();
    void processUdpTick();
    void updatePatientSettings(int id);
    void updateUiGraphs();

private:
    void updateBloodPressure(int id);
    double computePatientPhase(int id, qint64 now);
    double generateWaveform(int id, qint64 now);
    double getEcgComplex(double phase);
    double getGaussianPulse(double phase, double center, double width, double height);

    QMap<int, Patient> patients;
    QMap<int, QTcpSocket*> tcpSockets;
    QUdpSocket *udpSocket;

    bool isTcpStreaming = false;
    bool isUdpStreaming = false;

    QTimer *tcpTimer;
    QTimer *udpTimer;
    QTimer *uiTimer;

    // UI Pointers
    QMap<int, GraphWidget*> graphs;
    QMap<int, class QSlider*> hrSliders;
    QMap<int, class QSlider*> spo2Sliders;
    QMap<int, class QSlider*> rrSliders;
    QMap<int, class QSlider*> ampSliders;
    QMap<int, class QComboBox*> waveCombos;
    QMap<int, class QLabel*> hrLabels;
    QMap<int, class QLabel*> spo2Labels;
    QMap<int, class QLabel*> rrLabels;
    QMap<int, class QLabel*> ampLabels;
};

#endif // MAINWINDOW_H