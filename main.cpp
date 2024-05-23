
#include <QCoreApplication>
#include "Server.h"
#include <QSqlDatabase>


int main(int argc, char *argv[])
{

    QCoreApplication a(argc, argv);
   //CoreApplication::addLibraryPath ();
    //14736
    //19704
    Server server(&a , 19704);

    return a.exec();
}
