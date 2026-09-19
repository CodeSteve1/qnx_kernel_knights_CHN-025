// mainwindow.cpp
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QRandomGenerator>
#include <QPainter>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QDebug>
#include <cmath>

// ---------------------------------------------------------------------
// FIX: both TCP and UDP were pointing at two DIFFERENT hardcoded IPs
// ("10.11.10.253" for TCP, "10.11.11.3" for UDP). That mismatch is the
// most likely reason HR/vitals (TCP) never arrived while ECG (UDP) did.
// Set this ONE constant to the actual IP of the machine running monitor.c,
// and both sockets will use it.
// ---------------------------------------------------------------------
static const char* RECEIVER_IP = "10.42.0.41"; // <-- set this to your monitor.c host's IP
// ^^^ If every patient is failing with NetworkError/SocketTimeoutError, this
// is almost certainly the first thing to check: is this really the current
// IP of the machine running monitor.c, and is it reachable from this machine?

// --- GraphWidget Implementation ---
GraphWidget::GraphWidget(QWidget *parent) : QWidget(parent) {
    setFixedSize(300, 100);
    setStyleSheet("background-color: black; border: 1px solid #444;");
    for (int i = 0; i < maxPoints; ++i) history.append(0.0);
}

void GraphWidget::addPoint(double val) {
    history.pop_front();
    history.append(val);
}

void GraphWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw center line
    painter.setPen(QPen(QColor("#222")));
    painter.drawLine(0, 50, 300, 50);

    // Draw waveform
    painter.setPen(QPen(QColor("#00ff66"), 2));
    QPointF prev(0, 50 - (history[0] * 15));
    for (int i = 1; i < maxPoints; ++i) {
        double x = (static_cast<double>(i) / maxPoints) * 300.0;
        double y = 50.0 - (history[i] * 15.0);
        QPointF curr(x, y);
        painter.drawLine(prev, curr);
        prev = curr;
    }
}

// --- MainWindow Implementation ---
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("🎛️ Real-Time Waveform Controller & Latency Monitor");
    resize(1280, 720);

    udpSocket = new QUdpSocket(this);
    qint64 startupTime = QDateTime::currentMSecsSinceEpoch();

    qDebug() << "Configured RECEIVER_IP =" << RECEIVER_IP
              << "-- TCP ports 5001-5100, UDP ports 6001-6100";

    // Initialize 100 Patients
    for (int i = 1; i <= 100; ++i) {
        int hr0 = 70 + (i % 20);
        Patient p;
        p.id = i;
        p.heart_rate = hr0;
        p.spo2 = 98;
        p.rr = 16;
        p.is_critical = false;
        p.blood_pressure = "120/80";
        p.amplitude = 1.0;
        p.wave_type = "ecg";
        p.current_ecg = 0.0;
        p.cycle_start = startupTime;
        p.current_interval = 60.0 / hr0;
        p.sys = 120.0;
        p.dia = 80.0;
        patients[i] = p;

        QTcpSocket *sock = new QTcpSocket(this);
        sock->setSocketOption(QAbstractSocket::LowDelayOption, 1); // TCP_NODELAY

        // FIX: previously there was no visibility into connection failures.
        // A bad IP, unreachable host, or refused port would silently leave
        // the socket in UnconnectedState forever with no clue why.
        connect(sock, &QAbstractSocket::errorOccurred, this, [i](QAbstractSocket::SocketError err) {
            qWarning() << "Patient" << i << "TCP error:" << err;
        });
        connect(sock, &QTcpSocket::connected, this, [i]() {
            qDebug() << "Patient" << i << "TCP connected on port" << (5000 + i);
        });
        connect(sock, &QTcpSocket::disconnected, this, [i]() {
            qDebug() << "Patient" << i << "TCP disconnected";
        });

        tcpSockets[i] = sock;
    }

    // --- UI Setup ---
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Controls
    QWidget *controlWidget = new QWidget;
    QHBoxLayout *controlLayout = new QHBoxLayout(controlWidget);
    QPushButton *btnTcp = new QPushButton("▶ Start TCP (Vitals)");
    QPushButton *btnUdp = new QPushButton("▶ Start UDP (Live Graphs)");
    btnTcp->setStyleSheet("background-color: #00c853; color: black; font-weight: bold; padding: 10px;");
    btnUdp->setStyleSheet("background-color: #00c853; color: black; font-weight: bold; padding: 10px;");
    controlLayout->addWidget(btnTcp);
    controlLayout->addWidget(btnUdp);
    mainLayout->addWidget(controlWidget);

    connect(btnTcp, &QPushButton::clicked, this, &MainWindow::toggleTcp);
    connect(btnUdp, &QPushButton::clicked, this, &MainWindow::toggleUdp);

    // Grid for Patients
    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    QWidget *gridWidget = new QWidget;
    QGridLayout *gridLayout = new QGridLayout(gridWidget);

    int row = 0, col = 0;
    for (int i = 1; i <= 100; ++i) {
        QWidget *pWidget = new QWidget;
        pWidget->setStyleSheet("background-color: #1e1e1e; color: white; border-radius: 8px;");
        QVBoxLayout *pLayout = new QVBoxLayout(pWidget);

        QLabel *lblTitle = new QLabel(QString("Port %1 (Patient %2)").arg(6000 + i).arg(i));
        lblTitle->setStyleSheet("color: #00ff66; font-weight: bold;");
        pLayout->addWidget(lblTitle);

        // HR Slider
        QHBoxLayout *hrLayout = new QHBoxLayout;
        hrLabels[i] = new QLabel(QString::number(patients[i].heart_rate));
        hrSliders[i] = new QSlider(Qt::Horizontal);
        hrSliders[i]->setRange(10, 250);
        hrSliders[i]->setValue(patients[i].heart_rate);
        hrLayout->addWidget(new QLabel("HR:"));
        hrLayout->addWidget(hrLabels[i]);
        hrLayout->addWidget(hrSliders[i]);
        pLayout->addLayout(hrLayout);

        // SpO2 Slider
        QHBoxLayout *spo2Layout = new QHBoxLayout;
        spo2Labels[i] = new QLabel(QString::number(patients[i].spo2));
        spo2Sliders[i] = new QSlider(Qt::Horizontal);
        spo2Sliders[i]->setRange(70, 100);
        spo2Sliders[i]->setValue(patients[i].spo2);
        spo2Layout->addWidget(new QLabel("SpO2:"));
        spo2Layout->addWidget(spo2Labels[i]);
        spo2Layout->addWidget(spo2Sliders[i]);
        pLayout->addLayout(spo2Layout);

        // RR Slider
        QHBoxLayout *rrLayout = new QHBoxLayout;
        rrLabels[i] = new QLabel(QString::number(patients[i].rr));
        rrSliders[i] = new QSlider(Qt::Horizontal);
        rrSliders[i]->setRange(5, 40);
        rrSliders[i]->setValue(patients[i].rr);
        rrLayout->addWidget(new QLabel("RR:"));
        rrLayout->addWidget(rrLabels[i]);
        rrLayout->addWidget(rrSliders[i]);
        pLayout->addLayout(rrLayout);

        // Amplitude Slider
        QHBoxLayout *ampLayout = new QHBoxLayout;
        ampLabels[i] = new QLabel(QString::number(patients[i].amplitude, 'f', 1));
        ampSliders[i] = new QSlider(Qt::Horizontal);
        ampSliders[i]->setRange(1, 40); // 0.1 to 4.0
        ampSliders[i]->setValue(10);
        ampLayout->addWidget(new QLabel("Amp:"));
        ampLayout->addWidget(ampLabels[i]);
        ampLayout->addWidget(ampSliders[i]);
        pLayout->addLayout(ampLayout);

        // Wave Type Combo
        waveCombos[i] = new QComboBox;
        waveCombos[i]->addItem("Normal ECG", "ecg");
        waveCombos[i]->addItem("Sine Wave", "sine");
        waveCombos[i]->addItem("Square Wave", "square");
        waveCombos[i]->addItem("Flatline", "flat");
        waveCombos[i]->setStyleSheet("background: #333;");
        pLayout->addWidget(waveCombos[i]);

        // Graph
        graphs[i] = new GraphWidget;
        pLayout->addWidget(graphs[i]);

        gridLayout->addWidget(pWidget, row, col);
        col++;
        if (col > 3) { col = 0; row++; }

        // Connect UI changes
        connect(hrSliders[i], &QSlider::valueChanged, this, [this, i]() { updatePatientSettings(i); });
        connect(spo2Sliders[i], &QSlider::valueChanged, this, [this, i]() { updatePatientSettings(i); });
        connect(rrSliders[i], &QSlider::valueChanged, this, [this, i]() { updatePatientSettings(i); });
        connect(ampSliders[i], &QSlider::valueChanged, this, [this, i]() { updatePatientSettings(i); });
        connect(waveCombos[i], &QComboBox::currentIndexChanged, this, [this, i]() { updatePatientSettings(i); });
    }

    scrollArea->setWidget(gridWidget);
    mainLayout->addWidget(scrollArea);
    setCentralWidget(centralWidget);
    setStyleSheet("background-color: #121212;");

    // Timers
    tcpTimer = new QTimer(this);
    connect(tcpTimer, &QTimer::timeout, this, &MainWindow::processTcpTick);

    udpTimer = new QTimer(this);
    connect(udpTimer, &QTimer::timeout, this, &MainWindow::processUdpTick);

    uiTimer = new QTimer(this);
    connect(uiTimer, &QTimer::timeout, this, &MainWindow::updateUiGraphs);
    uiTimer->start(33); // ~30 FPS for UI redraws
}

MainWindow::~MainWindow() {}

void MainWindow::updatePatientSettings(int i) {
    patients[i].heart_rate = hrSliders[i]->value();
    hrLabels[i]->setText(QString::number(patients[i].heart_rate));

    patients[i].spo2 = spo2Sliders[i]->value();
    spo2Labels[i]->setText(QString::number(patients[i].spo2));

    patients[i].rr = rrSliders[i]->value();
    rrLabels[i]->setText(QString::number(patients[i].rr));

    patients[i].amplitude = ampSliders[i]->value() / 10.0;
    ampLabels[i]->setText(QString::number(patients[i].amplitude, 'f', 1));

    patients[i].wave_type = waveCombos[i]->currentData().toString();
}

void MainWindow::toggleTcp() {
    isTcpStreaming = !isTcpStreaming;
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (isTcpStreaming) {
        btn->setText("■ Stop TCP");
        btn->setStyleSheet("background-color: #d50000; color: white; font-weight: bold; padding: 10px;");
        tcpTimer->start(100); // 10 Hz
    } else {
        btn->setText("▶ Start TCP (Vitals)");
        btn->setStyleSheet("background-color: #00c853; color: black; font-weight: bold; padding: 10px;");
        tcpTimer->stop();
    }
}

void MainWindow::toggleUdp() {
    isUdpStreaming = !isUdpStreaming;
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (isUdpStreaming) {
        btn->setText("■ Stop UDP");
        btn->setStyleSheet("background-color: #d50000; color: white; font-weight: bold; padding: 10px;");
        udpTimer->start(10); // 100 Hz
    } else {
        btn->setText("▶ Start UDP (Live Graphs)");
        btn->setStyleSheet("background-color: #00c853; color: black; font-weight: bold; padding: 10px;");
        udpTimer->stop();
    }
}

void MainWindow::updateBloodPressure(int i) {
    Patient &p = patients[i];
    double hrDeviation = p.heart_rate - 70.0;
    double targetSys = 120.0 + hrDeviation * 0.3;
    double targetDia = 80.0 + hrDeviation * 0.15;

    double randSys = (QRandomGenerator::global()->generateDouble() * 0.8) - 0.4;
    double randDia = (QRandomGenerator::global()->generateDouble() * 0.6) - 0.3;

    p.sys = p.sys + (targetSys - p.sys) * 0.1 + randSys;
    p.dia = p.dia + (targetDia - p.dia) * 0.1 + randDia;

    p.sys = std::clamp(p.sys, 70.0, 220.0);
    p.dia = std::clamp(p.dia, 40.0, 140.0);
    if (p.dia > p.sys - 15) p.dia = p.sys - 15;

    p.blood_pressure = QString("%1/%2").arg(std::round(p.sys)).arg(std::round(p.dia));
}

void MainWindow::processTcpTick() {
    // FIX: this timer fires every 100ms. Previously, every single tick where
    // a socket was Unconnected called connectToHost() again -- that's 10
    // reconnect attempts per second per patient, which is both why the error
    // log was flooding so fast and unnecessary load while the real problem
    // (unreachable host) gets sorted out. Now it retries at most once every
    // 2 seconds per patient, and logs exactly what it's trying to reach.
    static QMap<int, qint64> lastAttempt;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (int i = 1; i <= 100; ++i) {
        QTcpSocket *sock = tcpSockets[i];
        if (sock->state() != QAbstractSocket::ConnectedState) {
            if (sock->state() == QAbstractSocket::UnconnectedState) {
                qint64 &last = lastAttempt[i];
                if (now - last >= 2000) {
                    last = now;
                    if (i == 1) { // one representative log line per round, not 100
                        qDebug() << "Attempting TCP connect, e.g. patient 1 ->"
                                  << RECEIVER_IP << ":" << (5000 + i);
                    }
                    sock->connectToHost(RECEIVER_IP, 5000 + i);
                }
            }
            continue;
        }

        updateBloodPressure(i);
        Patient &p = patients[i];

        // Critical Classification Logic
        p.is_critical = false;
        if (p.heart_rate < 50 || p.heart_rate > 120) p.is_critical = true;
        if (p.spo2 < 92) p.is_critical = true;
        if (p.rr < 10 || p.rr > 25) p.is_critical = true;
        if (p.sys > 180 || p.sys < 90) p.is_critical = true;
        if (p.dia > 120 || p.dia < 50) p.is_critical = true;
        if (p.wave_type == "flat") p.is_critical = true;

        QJsonObject json;
        json["heart_rate"] = p.heart_rate;
        json["spo2"] = p.spo2;
        json["rr"] = p.rr;
        json["blood_pressure"] = p.blood_pressure;
        json["critical"] = p.is_critical;
        json["timestamp"] = now;

        QJsonDocument doc(json);
        QByteArray payload = doc.toJson(QJsonDocument::Compact) + "\n";

        qint64 written = sock->write(payload);
        if (written == -1) {
            qWarning() << "Patient" << i << "TCP write failed:" << sock->errorString();
        }
    }
}

double MainWindow::getGaussianPulse(double phase, double center, double width, double height) {
    double dx = phase - center;
    return height * std::exp(-(dx * dx) / (2 * width * width));
}

double MainWindow::getEcgComplex(double phase) {
    return getGaussianPulse(phase, 0.10, 0.022, 0.18) +
           getGaussianPulse(phase, 0.195, 0.008, -0.12) +
           getGaussianPulse(phase, 0.21, 0.010, 1.35) +
           getGaussianPulse(phase, 0.23, 0.012, -0.30) +
           getGaussianPulse(phase, 0.42, 0.055, 0.35);
}

double MainWindow::computePatientPhase(int i, qint64 nowMS) {
    Patient &p = patients[i];
    double now = nowMS / 1000.0;
    int hr = p.heart_rate > 0 ? p.heart_rate : 60;

    // Fallback if data is fresh
    if (p.cycle_start > nowMS) p.cycle_start = nowMS;

    double elapsed = now - (p.cycle_start / 1000.0);
    double interval = p.current_interval;

    if (elapsed >= interval) {
        double baseInterval = 60.0 / hr;
        double jitter = (QRandomGenerator::global()->generateDouble() * 0.06) - 0.03;
        p.current_interval = std::max(0.05, baseInterval * (1.0 + jitter));
        p.cycle_start = nowMS;
        elapsed = 0.0;
        interval = p.current_interval;
    }

    double phase = (interval > 0) ? (elapsed / interval) : 0.0;
    return std::min(phase, 0.999999);
}

double MainWindow::generateWaveform(int i, qint64 nowMS) {
    Patient &p = patients[i];
    double phase = computePatientPhase(i, nowMS);
    double voltage = 0.0;

    if (p.wave_type == "sine") voltage = std::sin(phase * 2 * M_PI) * 1.5;
    else if (p.wave_type == "square") voltage = phase < 0.5 ? 1.5 : -1.5;
    else if (p.wave_type == "flat") voltage = 0.0;
    else voltage = getEcgComplex(phase);

    double noise = (QRandomGenerator::global()->generateDouble() * 0.04) - 0.02;
    return (voltage + noise) * p.amplitude;
}

void MainWindow::processUdpTick() {
    qint64 nowMS = QDateTime::currentMSecsSinceEpoch();
    for (int i = 1; i <= 100; ++i) {
        double val = generateWaveform(i, nowMS);
        patients[i].current_ecg = val;

        QJsonObject json;
        json["ecg"] = val;
        json["timestamp"] = nowMS;
        QByteArray payload = QJsonDocument(json).toJson(QJsonDocument::Compact);

        // FIX: now uses the same RECEIVER_IP as TCP instead of a
        // different hardcoded address.
        udpSocket->writeDatagram(payload, QHostAddress(RECEIVER_IP), 6000 + i);
    }
}

void MainWindow::updateUiGraphs() {
    if (!isUdpStreaming) return;
    for (int i = 1; i <= 100; ++i) {
        graphs[i]->addPoint(patients[i].current_ecg);
        graphs[i]->update();
    }
}