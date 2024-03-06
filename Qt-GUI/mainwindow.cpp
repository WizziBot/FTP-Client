#include "mainwindow.h"
#include "terminaloutput.h"
#include "ui_mainwindow.h"
#include <string>
#include <QObject>
#include <iostream>
#include <string>

MainWindow* w_ref;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    this->setFixedSize(QSize(1097, 773));
    w_ref = this;
    ui->setupUi(this);
    QObject::connect(ui->connect_btn,&QPushButton::clicked,this,&MainWindow::connect);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::pushText(std::string output){
    ui->o_control_text->setTerminalText("\n" + QString::fromStdString(output));
}

void logger(std::string text) {
    w_ref->pushText(text);
}

void MainWindow::connect()
{

    using namespace FTP;
    using namespace std;

    string address = ui->i_address->toPlainText().toStdString();
    string username = ui->i_username->toPlainText().toStdString();
    string password = ui->i_password->toPlainText().toStdString();
    
    Conn = new ControlConnection(address);
    Conn->setLogger(logger);
    if (Conn->initConnection() == -1){
        pushText(string("Unable to start connection"));
        return;
    }

    string response = Conn->ftp_login(username,password);
    pushText(response);

    if (response.at(0) != D1_COMPLETION){
        delete Conn;
    }
}
