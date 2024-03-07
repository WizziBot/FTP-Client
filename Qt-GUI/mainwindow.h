
#pragma once

#include <QListWidgetItem>
#include <QMainWindow>
#include <vector>
#include <memory>
#include <utility>
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
    void localDirectoryChange(QListWidgetItem* item);
    void remoteDirectoryChange(QListWidgetItem* item);

private:
    Ui::MainWindow *ui;
    FTP::ControlConnection* Conn;
    std::string current_directory;
    std::string current_remote_directory;
    // Entry: QListWidgetItem, isDirectory flag
    std::vector<std::pair<std::unique_ptr<QListWidgetItem>,bool>> local_files;
    std::vector<std::pair<std::unique_ptr<QListWidgetItem>,bool>> remote_files;
};

