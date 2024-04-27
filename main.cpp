
#include <QCoreApplication>
#include "Server.h"
#include <QSqlDatabase>


int main(int argc, char *argv[])
{

    QCoreApplication a(argc, argv);
   //CoreApplication::addLibraryPath ();
    Server server(&a , 19704);

    return a.exec();
}
