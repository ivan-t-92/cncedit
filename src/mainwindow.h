#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

#include <QMainWindow>

class DocumentView;
class QTabWidget;
class QTextDocument;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    void openFile(const QString& path);

private slots:
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionClose_triggered();
    void on_actionExit_triggered();
    void onDocumentModificationChange(bool);
    void onDocumentChange();
    void on_tabWidget_tabCloseRequested(int index);
    void on_tabWidget_currentChanged(int index);

private:

    void saveDocument(int index, bool saveAs);
    void closeTab(int index);
    QTextDocument* currentDocument() const;
    QTextDocument* documentAt(int index) const;

    void adoptUIToDocument(int index);
    DocumentView* createNewView(const QString& text = QString());
};
#endif // MAINWINDOW_H
