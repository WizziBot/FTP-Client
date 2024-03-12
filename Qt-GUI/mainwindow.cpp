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
    QObject::connect(ui->f_server,&QListWidget::itemClicked,this,&MainWindow::updateFileInfo);
    QObject::connect(ui->stor_btn,&QPushButton::clicked,this,&MainWindow::storCommand);
    QObject::connect(ui->recv_btn,&QPushButton::clicked,this,&MainWindow::retrCommand);
    QObject::connect(ui->dele_btn,&QPushButton::clicked,this,&MainWindow::deleCommand);
    QObject::connect(ui->mkdir_btn,&QPushButton::clicked,this,&MainWindow::mkdCommand);
    QObject::connect(ui->rmd_btn,&QPushButton::clicked,this,&MainWindow::rmdCommand);
    QObject::connect(ui->switch_type_btn,&QPushButton::clicked,this,&MainWindow::typeCommand);

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

string trimTrailingWhitespace(const std::string& str) {
    size_t endpos = str.find_last_not_of("'\t\r\n");
    return (endpos != string::npos) ? str.substr(0, endpos + 1) : "NameResolutionError";
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
    struct fileinfo back_file;
    back_file.is_dir = 1;
    back_file.size = 0;
    remote_files.push_back(make_pair<unique_ptr<QListWidgetItem>,struct fileinfo>(std::move(file),(struct fileinfo)back_file));

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
        QString f_name = QString::fromStdString(trimTrailingWhitespace(string(entry.name,entry.namelen)));

        // Construct List Entry
        unique_ptr<QListWidgetItem> file;
        if (entry.flagtrycwd == 1) {
            file = make_unique<QListWidgetItem>(dir_icon,f_name);
        } else {
            file = make_unique<QListWidgetItem>(file_icon,f_name);
        }

        struct fileinfo finfo;
        finfo.is_dir = entry.flagtrycwd;
        finfo.size = entry.size;
        // Update listing
        ui->f_server->addItem(file.get());
        remote_files.push_back(make_pair<unique_ptr<QListWidgetItem>,struct fileinfo > (std::move(file),(struct fileinfo)finfo));
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
                               [&item](const std::pair<unique_ptr<QListWidgetItem>,struct fileinfo>& i_item)
                               {return (i_item.first->text() == item->text());});

    if (remote_file == remote_files.end()) return;
    if (!remote_file->second.is_dir) return; // If file is not a directory, ignore.

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
    if (!Conn || Conn->getConStatus() != CONN_SUCCESS) return;

    // Get local file to be sent from QListWidget
    QListWidgetItem* selected = ui->f_host->currentItem();
    if (!selected) return; // Must select file.
    auto local_file = std::find_if(local_files.begin(),local_files.end(),
                               [&selected](const std::pair<unique_ptr<QListWidgetItem>,bool>& i_item)
                               {return (i_item.first->text() == selected->text());});
    if (local_file->second) return; // Cant send directories.
    string filename = local_file->first->text().toStdString();

    // Set Progress bar label to file name

    ui->l_file_name->setText(local_file->first->text());

    int status = Conn->stor(filename,filename);
    if(status == -1){
        return;
    } else if (status == -2) {
        pushText(Conn->getLastResponse());
        return;
    }

    int t_progress = Conn->getTranferProgress();
    while (t_progress < 100 && Conn->isTrasferInProgress()) {
        ui->p_file_status->setValue(t_progress);
        t_progress = Conn->getTranferProgress();
    }
    // Atomic attribute acts as semaphore in ControlConnection
    // Waits for termination of whole transfer operation including the wrapper lambda.
    while (Conn->isTrasferInProgress()) {};

    ui->p_file_status->setValue(100);
    pushText(Conn->getLastResponse());

    updateRemoteDirectoryListing();
}

void MainWindow::retrCommand(){
    if (!Conn || Conn->getConStatus() != CONN_SUCCESS) return;

    QListWidgetItem* selected = ui->f_server->currentItem();
    if (!selected) return;
    auto remote_file = std::find_if(remote_files.begin(),remote_files.end(),
                               [&selected](const std::pair<unique_ptr<QListWidgetItem>,struct fileinfo>& i_item)
                               {return (i_item.first->text() == selected->text());});
    if (remote_file->second.is_dir) return;
    string filename = remote_file->first->text().toStdString();
    
    ui->l_file_name->setText(remote_file->first->text());

    int status = Conn->retr(filename,filename,remote_file->second.size);
    if(status == -1){
        return;
    } else if (status == -2) {
        pushText(Conn->getLastResponse());
        return;
    }

    int t_progress = Conn->getTranferProgress();
    while (t_progress < 100 && Conn->isTrasferInProgress()) {
        ui->p_file_status->setValue(t_progress);
        t_progress = Conn->getTranferProgress();
    }

    while (Conn->isTrasferInProgress()) {}; 

    ui->p_file_status->setValue(100);
    pushText(Conn->getLastResponse());


    updateLocalDirectoryListing();
}

void MainWindow::updateFileInfo(QListWidgetItem* item) {
    if (!Conn || Conn->getConStatus() != CONN_SUCCESS) return;

    QListWidgetItem* selected = ui->f_server->currentItem();
    if (!selected) return;
    auto remote_file = std::find_if(remote_files.begin(),remote_files.end(),
                               [&selected](const std::pair<unique_ptr<QListWidgetItem>,struct fileinfo>& i_item)
                               {return (i_item.first->text() == selected->text());});
    string filename =  "File Name: "+remote_file->first->text().toStdString();
    string is_directory = "Is Directory: ";
    string file_size = "File Size: ";
    if (remote_file->second.is_dir){
        is_directory += "Yes";
        file_size += to_string(0);
    } else {
        is_directory += "No";
        file_size += to_string(remote_file->second.size);
    }

    // Edit file information widget

    ui->f_remote_info->clear();
    ui->f_remote_info->addItem(QString::fromStdString(filename));
    ui->f_remote_info->addItem(QString::fromStdString(is_directory));
    ui->f_remote_info->addItem(QString::fromStdString(file_size));
}

void MainWindow::deleCommand(){
    if (!Conn || Conn->getConStatus() != CONN_SUCCESS) return;

    QListWidgetItem* selected = ui->f_server->currentItem();
    if (!selected) return;
    auto remote_file = std::find_if(remote_files.begin(),remote_files.end(),
                               [&selected](const std::pair<unique_ptr<QListWidgetItem>,struct fileinfo>& i_item)
                               {return (i_item.first->text() == selected->text());});
    if (remote_file->second.is_dir) return;
    
    // Send delete request
    string response = Conn->dele(remote_file->first->text().toStdString());
    pushText(response);

    updateRemoteDirectoryListing();
}

void MainWindow::mkdCommand(){
    if (!Conn || Conn->getConStatus() != CONN_SUCCESS) return;

    string dir_name = ui->i_newdir->toPlainText().toStdString();
    ui->i_newdir->setText("");
    if (dir_name == "") return; // Cannot be empty
    
    // Send rmdir request
    string response = Conn->mkd(dir_name);
    pushText(response);

    updateRemoteDirectoryListing();
}

void MainWindow::rmdCommand(){
    if (!Conn || Conn->getConStatus() != CONN_SUCCESS) return;
    
    QListWidgetItem* selected = ui->f_server->currentItem();
    if (!selected) return;
    auto remote_file = std::find_if(remote_files.begin(),remote_files.end(),
                               [&selected](const std::pair<unique_ptr<QListWidgetItem>,struct fileinfo>& i_item)
                               {return (i_item.first->text() == selected->text());});
    if (!remote_file->second.is_dir) return;
    
    // Send rmdir request
    string response = Conn->rmd(remote_file->first->text().toStdString());
    pushText(response);

    updateRemoteDirectoryListing();
}

void MainWindow::typeCommand(){
    if (!Conn || Conn->getConStatus() != CONN_SUCCESS) return;
    string label_text = "Current: ";
    string response;
    if (Conn->getDataType() == DATA_ASCII) {
        response = Conn->type("binary");
        if (response.at(0) == D1_COMPLETION){
            label_text += "BINARY";
        }
    } else {
        response = Conn->type("ascii");
        if (response.at(0) == D1_COMPLETION){
            label_text += "ASCII";
        }
    }
    pushText(response);
    ui->l_current_type->setText(QString::fromStdString(label_text));
}
