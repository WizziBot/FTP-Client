
#pragma once

#include <QMainWindow>
#include <StatusConstants.h>
#include <ControlConnection.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void pushText(std::string output);

private slots:
    void connect();

private:
    Ui::MainWindow *ui;
    FTP::ControlConnection* Conn;
};

