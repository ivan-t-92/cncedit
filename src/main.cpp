#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QSurfaceFormat fmt;
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4);
    fmt.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(fmt);

    MainWindow w;
    QCommandLineParser parser;
    parser.process(app);
    if (!parser.positionalArguments().isEmpty())
        w.openFile(parser.positionalArguments().first());

    w.resize(w.sizeHint());
    w.show();
    return QApplication::exec();
}
