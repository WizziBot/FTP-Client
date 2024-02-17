#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <ControlConnection.h>
#include <QObject>
#include <iostream>
#include <string>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->sendCommandBtn,&QPushButton::clicked,this,&MainWindow::sendCommand);
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
    ControlConnection Conn1("127.0.0.1");
    if (Conn1.initConnection() == -1){
        pushText(std::string("Unable to start connection"));
    }
}

void MainWindow::sendCommand(bool clicked){
    using namespace std;

    string command = ui->commandInput->toPlainText().toStdString();
    std::cout << command << std::endl;

}
