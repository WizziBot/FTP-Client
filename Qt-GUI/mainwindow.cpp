#include "mainwindow.h"
#include "terminaloutput.h"
#include "ui_mainwindow.h"
#include "ftpparse.h"
#include <string>
#include <QObject>
#include <QIcon>
#include <QFileIconProvider>
#include <iostream>
#include <strstream>
#include <string>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
using namespace std;

QIcon dir_icon;
QIcon file_icon;

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

    QFileIconProvider provider;
    dir_icon = provider.icon(QFileIconProvider::Folder);
    file_icon = provider.icon(QFileIconProvider::File);

    updateLocalDirectoryListing();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateLocalDirectoryListing() {
    local_files.clear();
    for (const auto & entry : fs::directory_iterator(current_directory)){
        // Acquire file name
        QString f_name = QString::fromStdString(fs::path(entry).filename().string());

        // Construct List Entry
        unique_ptr<QListWidgetItem> file;
        if (fs::is_directory(entry)) {
            file = make_unique<QListWidgetItem>(dir_icon,f_name);
        } else {
            file = make_unique<QListWidgetItem>(file_icon,f_name);
        }

        // Update listing
        ui->f_host->addItem(file.get());
        local_files.push_back(std::move(file));
    }
}

void MainWindow::updateRemoteDirectoryListing() {

    // Retreive directory listing
    string listing = Conn->list();

    if (Conn->getLastResponse().at(0) != D1_COMPLETION){
        return; // Directory listing was not succesfully transferred.
    }

    // Separate dir listing into lines
    vector<string> lines;
    stringstream ss(listing);
    string temp;
    while(getline(ss,temp,'\n')){
        lines.push_back(temp);
    }

    vector<struct ftpparse> directories;
    for (const string line : lines){
        // Acquire file name
        struct ftpparse entry;
        ftpparse(&entry,(char*)line.c_str(),line.length());
        QString f_name = QString(entry.name);

        // Construct List Entry
        unique_ptr<QListWidgetItem> file;
        if (entry.flagtrycwd == 1) {
            file = make_unique<QListWidgetItem>(dir_icon,f_name);
        } else {
            file = make_unique<QListWidgetItem>(file_icon,f_name);
        }

        // Update listing
        ui->f_server->addItem(file.get());
        remote_files.push_back(std::move(file));
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

    updateRemoteDirectoryListing();
}
