
#pragma once

#include <QListWidgetItem>
#include <QMainWindow>
#include <vector>
#include <memory>
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

    void updateLocalDirectoryListing();
    void updateRemoteDirectoryListing();

private slots:
    void connect();

private:
    Ui::MainWindow *ui;
    FTP::ControlConnection* Conn;
    std::string current_directory;
    std::vector<std::unique_ptr<QListWidgetItem>> local_files;
    std::vector<std::unique_ptr<QListWidgetItem>> remote_files;
};

