#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ftpparse.h"
#include <string>
#include <QObject>
#include <QIcon>
#include <QFileIconProvider>
#include <iostream>
#include <strstream>
#include <string>
#include <algorithm>
#include <time.h>
#include <StatusConstants.h>
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
    ui->p_file_status->setMinimum(0);
    ui->p_file_status->setMaximum(100);
    ui->p_file_status->setValue(0);

    QObject::connect(ui->connect_btn,&QPushButton::clicked,this,&MainWindow::connect);

    QObject::connect(ui->f_host,&QListWidget::itemDoubleClicked,this,&MainWindow::localDirectoryChange);
    QObject::connect(ui->f_server,&QListWidget::itemDoubleClicked,this,&MainWindow::remoteDirectoryChange);
    QObject::connect(ui->stor_btn,&QPushButton::clicked,this,&MainWindow::storCommand);
    QObject::connect(ui->recv_btn,&QPushButton::clicked,this,&MainWindow::retrCommand);

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
    // Add listing for parent directory (..)
    unique_ptr<QListWidgetItem> file = make_unique<QListWidgetItem>(dir_icon,"..");
    ui->f_host->addItem(file.get());
    local_files.push_back(make_pair<unique_ptr<QListWidgetItem>,bool>(std::move(file),true));
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
        local_files.push_back(make_pair<unique_ptr<QListWidgetItem>,bool>(std::move(file),fs::is_directory(entry)));
    }
}

void MainWindow::updateRemoteDirectoryListing() {
    remote_files.clear();
    // Retreive directory listing
    string listing = Conn->list();

    if (Conn->getLastResponse().at(0) != D1_COMPLETION){
        return; // Directory listing was not succesfully transferred.
    }

    // Add listing for parent directory (..)
    unique_ptr<QListWidgetItem> file = make_unique<QListWidgetItem>(dir_icon,"..");
    ui->f_server->addItem(file.get());
    remote_files.push_back(make_pair<unique_ptr<QListWidgetItem>,bool>(std::move(file),true));

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
        remote_files.push_back(make_pair<unique_ptr<QListWidgetItem>,bool>(std::move(file),entry.flagtrycwd == 1));
    }

}

// Return time formatted HH:MM:SS
const string currentTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%X", &tstruct);
    return buf;
}

// Append text to control connection terminal output.
void MainWindow::pushText(std::string output){
    ui->o_control_text->setTerminalText(QString::fromStdString("\n" + currentTime() + " " + output));
}

void logger(std::string text) {
    w_ref->pushText(text);
}

// Slots

void MainWindow::localDirectoryChange(QListWidgetItem* item){
    auto local_file = std::find_if(local_files.begin(),local_files.end(),
                               [&item](const std::pair<unique_ptr<QListWidgetItem>,bool>& i_item)
                               {return (i_item.first->text() == item->text());});

    if (local_file == local_files.end()) return; // Should not occur, ever. The item should be in local_files.
    if (!local_file->second) return; // If file is not a directory, ignore.

    if(local_file->first->text() == ".."){
        current_directory = fs::path(current_directory).parent_path().string(); // Go to parent directory
    } else {
        current_directory = current_directory + "/" + item->text().toStdString();
    }
    updateLocalDirectoryListing();
}

void MainWindow::remoteDirectoryChange(QListWidgetItem* item){
    auto remote_file = std::find_if(remote_files.begin(),remote_files.end(),
                               [&item](const std::pair<unique_ptr<QListWidgetItem>,bool>& i_item)
                               {return (i_item.first->text() == item->text());});

    if (remote_file == remote_files.end()) return;
    if (!remote_file->second) return; // If file is not a directory, ignore.

    string response = Conn->cwd(remote_file->first->text().toStdString());
    if (response.at(0) != D1_COMPLETION) { // If directory change failed, do not proceed.
        // Update directory anyway in case file deletion caused failure.
        updateRemoteDirectoryListing();
        return;
    };

    updateRemoteDirectoryListing();
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

void MainWindow::connect()
{

    string address = ui->i_address->toPlainText().toStdString();
    string port_str = ui->i_port->toPlainText().toStdString();
    string username = ui->i_username->toPlainText().toStdString();
    string password = ui->i_password->toPlainText().toStdString();

    if (is_number(port_str)){
        Conn = new FTP::ControlConnection(address,atoi(port_str.c_str()));
    } else {
        Conn = new FTP::ControlConnection(address);
    }
    
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

void MainWindow::storCommand(){
    // Get local file to be sent from QListWidget
    QListWidgetItem* selected = ui->f_host->currentItem();
    auto local_file = std::find_if(local_files.begin(),local_files.end(),
                               [&selected](const std::pair<unique_ptr<QListWidgetItem>,bool>& i_item)
                               {return (i_item.first->text() == selected->text());});
    if (local_file->second) return; // Cant send directories.
    string filename = local_file->first->text().toStdString();

    // Set Progress bar label to file name

    ui->l_file_name->setText(local_file->first->text());

    // Always use binary mode to keep transfers simple.

    if (Conn->getTransferType() != DATA_BINARY){
        pushText(Conn->type("binary"));
    }

    int status = Conn->stor(filename,filename);
    if(status == -1){
        return;
    } else if (status == -2) {
        pushText(Conn->getLastResponse());
        return;
    }

    if (Conn->getTranferProgress() == -1) return;
    int t_progress = Conn->getTranferProgress();
    while (t_progress < 100) {
        ui->p_file_status->setValue(t_progress);
        t_progress = Conn->getTranferProgress();
    }
    ui->p_file_status->setValue(100);
    // Atomic attribute acts as semaphore in ControlConnection
    // Waits for termination of whole transfer operation including the wrapper lambda.
    while (Conn->isTrasferInProgress()) {}; 

    pushText(Conn->getLastResponse());

    updateRemoteDirectoryListing();
}

void MainWindow::retrCommand(){
    // Get remote file to be sent from QListWidget
    
    // Get file name and put in retr() FTP command

    // Update local dir listing
}
