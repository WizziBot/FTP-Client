/*  FTP Client
    Copyright (C) 2023  Matthew Rodriguez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <QListWidgetItem>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <vector>
#include <memory>
#include <utility>
#include <StatusConstants.h>
#include <ControlConnection.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
class TerminalOutput;
}
QT_END_NAMESPACE

struct fileinfo {
  int is_dir;
  long size;
};

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
    void storCommand();
    void retrCommand();
    void updateFileInfo(QListWidgetItem* item);
    void deleCommand();
    void mkdCommand();
    void rmdCommand();
    void typeCommand();

private:
    Ui::MainWindow *ui;
    FTP::ControlConnection* Conn = nullptr;
    std::string current_directory;
    // Entry: QListWidgetItem, isDirectory flag
    std::vector<std::pair<std::unique_ptr<QListWidgetItem>,bool>> local_files;
    std::vector<std::pair<std::unique_ptr<QListWidgetItem>,struct fileinfo>> remote_files;
};

class TerminalOutput : public QPlainTextEdit
{
    Q_OBJECT

public:
    TerminalOutput(QWidget *parent = nullptr) : QPlainTextEdit(parent) {
        setReadOnly(true);
    }

    void setTerminalText(const QString& text) {
        // Scroll to bottom of box and append text.
        moveCursor(QTextCursor::End);
        insertPlainText(text);
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
};

