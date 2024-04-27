
#ifndef UTILITY_H
#define UTILITY_H
#include <QTcpSocket>

namespace Utility{
QString getClientKey(const QTcpSocket* client);
}
#endif // UTILITY_H
