#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "codeeditor.h"
#include "documentview.h"

#include <QTabWidget>
#include <QKeySequence>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QTextCodec>


constexpr std::array DEFAULT_NAME {"Untitled"};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);

    tabWidget->setMovable(true);
    tabWidget->setTabsClosable(true);
    on_actionNew_triggered();
}

QSize MainWindow::sizeHint() const
{
    return QSize{1600, 900};
}

void MainWindow::openFile(const QString& path)
{
    const QFileInfo fileInfo {path};
    for (int i = 0; i < tabWidget->count(); i++)
    {
        if (path == documentAt(i)->metaInformation(QTextDocument::DocumentUrl))
        {
            tabWidget->setCurrentIndex(i);
            return;
        }
    }

    QFile file {path};
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream textStream {&file};
    textStream.setCodec(QTextCodec::codecForName("UTF-8"));

    DocumentView* view {createNewView(textStream.readAll())};
    view->document()->setMetaInformation(QTextDocument::DocumentUrl, path);
    file.close();

    // remove that one default empty unmodified tab
    if (tabWidget->count() == 1 &&
        documentAt(0)->metaInformation(QTextDocument::DocumentUrl).isEmpty() &&
        documentAt(0)->isEmpty() &&
        !documentAt(0)->isModified())
    {
        tabWidget->removeTab(0);
    }

    const int tabIndex = tabWidget->addTab(view, fileInfo.fileName());
    tabWidget->setCurrentIndex(tabIndex);
}

void MainWindow::saveDocument(int index, bool saveAs)
{
    QString path = documentAt(index)->metaInformation(QTextDocument::DocumentUrl);
    if (saveAs || path.isEmpty())
    {
        QFileDialog dialog(this);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        if (!path.isEmpty())
            dialog.selectFile(path);

        if (dialog.exec())
        {
            QStringList fileNames = dialog.selectedFiles();
            path = fileNames.first();
        }
        currentDocument()->setMetaInformation(QTextDocument::DocumentUrl, path);
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream stream(&file);
    stream << currentDocument()->toPlainText();

    file.close();

    currentDocument()->setModified(false);
}

void MainWindow::closeTab(int index)
{
    bool closeTab = true;
    if (documentAt(index)->isModified())
    {
        const QMessageBox::StandardButton button =
                QMessageBox::warning(this,
                                     tr("The document has been modified."),
                                     tr("Do you want to save your changes?"),
                                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                     QMessageBox::Save);

        switch (button) {
        case QMessageBox::Save:
            saveDocument(index, false);
            break;
        case QMessageBox::Cancel:
            closeTab = false;
            break;
        default:
            ;
        }
    }

    if (closeTab)
    {
        QWidget* widget = tabWidget->widget(index);
        tabWidget->removeTab(index);
        delete widget;
    }
}

QTextDocument* MainWindow::documentAt(int index) const
{
    return (dynamic_cast<DocumentView*>( tabWidget->widget(index)))->document();
}

void MainWindow::adoptUIToDocument(int index)
{
    actionSave->setEnabled(documentAt(index)->isModified());

    QString path = currentDocument()->metaInformation(QTextDocument::DocumentUrl);
    QString tabText = path.length() > 0 ?
                      QFileInfo(path).fileName() :
                      *DEFAULT_NAME.data();

    if (documentAt(index)->isModified())
        tabText += " *";

    tabWidget->setTabText(tabWidget->currentIndex(), tabText);
}

DocumentView* MainWindow::createNewView(const QString& text)
{
    auto view {new DocumentView(text)};
    connect(view->document(), &QTextDocument::modificationChanged, this, &MainWindow::onDocumentModificationChange);
    connect(view->document(), &QTextDocument::contentsChanged, this, &MainWindow::onDocumentChange);

    return view;
}

QTextDocument* MainWindow::currentDocument() const
{
    return (dynamic_cast<DocumentView*>(tabWidget->currentWidget()))->document();
}


void MainWindow::on_actionNew_triggered()
{
    DocumentView* editor {createNewView()};
    int index {tabWidget->addTab(editor, *DEFAULT_NAME.data())};
    tabWidget->setCurrentIndex(index);
}

void MainWindow::on_actionOpen_triggered()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    if (dialog.exec())
    {
        QStringList fileNames = dialog.selectedFiles();
        QStringListIterator iter(fileNames);
        while (iter.hasNext())
            openFile(iter.next());
    }
}

void MainWindow::on_actionSave_triggered()
{
    saveDocument(tabWidget->currentIndex(), false);
}

void MainWindow::on_actionSaveAs_triggered()
{
    saveDocument(tabWidget->currentIndex(), true);
}

void MainWindow::on_actionClose_triggered()
{
    closeTab(tabWidget->currentIndex());
}

void MainWindow::on_actionExit_triggered()
{
    //TODO: check for modified
    QApplication::quit();
}

void MainWindow::onDocumentModificationChange(bool)
{
    adoptUIToDocument(tabWidget->currentIndex());
}

void MainWindow::onDocumentChange()
{
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    closeTab(index);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index == -1)
        return;

    adoptUIToDocument(index);
}
