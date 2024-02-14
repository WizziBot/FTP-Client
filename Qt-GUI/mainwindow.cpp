#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <ControlConnection.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::pushText(std::string output){

    ui->outputText->setText(QString::fromStdString(output));
}

void MainWindow::on_pushButton_clicked()
{
    using namespace FTP;
    // using std::string;
    // using std::to_string;
    // ui->outputText->setText(QString::fromStdString(string("Pressed ") + to_string(countPressed) + string(" times")));
    // countPressed++;
    ControlConnection Conn1("127.0.0.1");
    if (Conn1.initConnection() == -1){
        pushText(std::string("Unable to start connection"));
    }
}

