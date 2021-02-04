#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mpeg2ts.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
signals:
    void mpeg2tsStart();
private:
    Ui::MainWindow *ui;
    mpeg2ts* m_pMpeg2ts;

};

#endif // MAINWINDOW_H
