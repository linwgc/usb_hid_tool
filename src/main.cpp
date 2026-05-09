#include "ui/main_window.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("air_race_hid_tool"));
    QCoreApplication::setOrganizationName(QStringLiteral("AUT"));

    MainWindow w;
    w.show();
    return app.exec();
}
