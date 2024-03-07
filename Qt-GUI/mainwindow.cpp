#include "mainwindow.h"
#include "terminaloutput.h"
#include "ui_mainwindow.h"
#include "ftpparse.h"
#include <string>
#include <QObject>
#include <QIcon>
#include <iostream>
#include <strstream>
#include <string>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
using namespace std;

QIcon dir_icon = QIcon::fromTheme("folder");
QIcon file_icon = QIcon::fromTheme("text-x-generic");

MainWindow* w_ref;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    this->setFixedSize(QSize(1097, 773));
    w_ref = this;
    ui->setupUi(this);
    QObject::connect(ui->connect_btn,&QPushButton::clicked,this,&MainWindow::connect);

    current_directory = fs::current_path();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateLocalDirectoryListing() {
    cout << "---LOCAL---" << endl;
    for (const auto & entry : fs::directory_iterator(current_directory)){
        // iterate each file and insert into the dir listings (f_host, f_server)
        cout << fs::path(entry).filename().string();
        if (fs::is_directory(entry)) {
            cout << "(DIR)" << endl;
        } else {
            cout << "(FILE)" << endl;
        }
    }
}

void MainWindow::updateRemoteDirectoryListing() {

    // Retreive directory listing
    string listing = Conn->list();

    // Separate dir listing into lines
    vector<string> lines;
    stringstream ss(listing);
    string temp;
    while(getline(ss,temp,'\n')){
        lines.push_back(temp);
    }

    // run ftpparse on each line to extract file names
    vector<struct ftpparse> directories;
    cout << "---REMOTE---" << endl;
    for (const string line : lines){
        struct ftpparse entry;
        ftpparse(&entry,(char*)line.c_str(),line.length());
        cout << entry.name <<endl;
        if (entry.flagtrycwd == 1) {
            cout << "(DIR)" << endl;
        } else {
            cout << "(FILE)" << endl;
        }
    }

}

void MainWindow::pushText(std::string output){
    ui->o_control_text->setTerminalText("\n" + QString::fromStdString(output));
}

void logger(std::string text) {
    w_ref->pushText(text);
}

void MainWindow::connect()
{

    string address = ui->i_address->toPlainText().toStdString();
    string username = ui->i_username->toPlainText().toStdString();
    string password = ui->i_password->toPlainText().toStdString();
    
    Conn = new FTP::ControlConnection(address);
    Conn->setLogger(logger);
    if (Conn->initConnection() == -1){
        pushText(string("Unable to start connection"));
        return;
    }

    string response = Conn->ftp_login(username,password);
    pushText(response);

    if (response.at(0) != D1_COMPLETION){
        delete Conn;
        return;
    }

    //testcommand
    // Conn->cwd("ftp");

    updateLocalDirectoryListing();
    updateRemoteDirectoryListing();
}
