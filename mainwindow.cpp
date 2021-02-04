#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QThread>
#include <QDebug>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug()<<"main thread: tid  = "<<QThread::currentThreadId();
    QThread *thread = new QThread(this);
    m_pMpeg2ts = new mpeg2ts(this);
    m_pMpeg2ts->moveToThread(thread);
    QObject::connect(this, &MainWindow::mpeg2tsStart, m_pMpeg2ts, &mpeg2ts::_start, Qt::QueuedConnection);
    thread->start();
    emit this->mpeg2tsStart();
}

MainWindow::~MainWindow()
{
    delete ui;
}
