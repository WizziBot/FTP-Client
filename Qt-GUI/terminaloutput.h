
#pragma once

#include <QPlainTextEdit>
#include <QScrollBar>

QT_BEGIN_NAMESPACE
namespace Ui {
class TerminalOutput;
}
QT_END_NAMESPACE

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