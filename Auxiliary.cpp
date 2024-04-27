#include "Auxiliary.h"


QString Auxiliary::getClientKey(const QTcpSocket* client)
{
    return client->peerAddress().toString().append(':').append(QString::number(client->peerPort()));
}
QString Auxiliary::quoteIt(const QString& str)
{
    return '"' + str + '"';
}
QString Auxiliary::safeQuoteIt(QString str)
{
    return " \"" + str.replace('"', "\"") + " \"";
}

void Auxiliary::createCmd(QString& info , InfoToClient type)
{
    info = commandBegin + "I" + QString::number(int(type)) + ':' +  info + commandEnd;
}
void Auxiliary::createCmd(QString& info, RequestFailure type)
{
    info = commandBegin + "F" + QString::number(int(type)) + ':' + info + commandEnd;
}

QString Auxiliary::makeList(std::vector<QString> list, char sepBegin , char sepEnd)
{
    QString str = QChar(sepBegin);
    for (int i = 0; i < list.size(); i++)
        str += list[i] + ',';
    str.back() = sepEnd;
    return str;
}

std::vector<int> Auxiliary::extractIntArray(const QString& str)
{
    std::vector<int> memberList;
    QString nrStr;
    qint16 i = str.indexOf('{') + 1;

    while (str[i] != '}')
    {
        if (str[i] == ',')
        {
            memberList.emplace_back(nrStr.toInt());
            nrStr = "";
        }
        else
            nrStr += str[i];
        i++;
    }
    memberList.emplace_back(nrStr.toInt());
    return memberList;
}

QDateTime Auxiliary::stringToDateTime(const QString& str)
{
    int timeStampPos = 0;
    std::vector<int> date(3);
    std::vector<int> time(3);
    int i = 0;
    QString aux;
    //ok so for some reason , when I read from the database and send it to client , the separator between date and time is sometimes 'T' and sometimes ' '
    while (str[i] != ' ')
    {
        if (str[i] == '-')
        {
            date[timeStampPos] = aux.toInt();
            timeStampPos++;
            aux = "";
        }
        else
            aux += str[i];
        i++;
    }
    date[timeStampPos] = aux.toInt();
    timeStampPos = 0;
    aux = "";
    i++;
    while (i < str.length())
    {
        if (str[i] == ':')
        {
            time[timeStampPos] = aux.toInt();
            timeStampPos++;
            aux = "";
        }
        else
            aux += str[i];
        i++;
    }
    return QDateTime(QDate(date[0], date[1], date[2]), QTime(time[0], time[1], time[2]), QTimeZone(QTimeZone::LocalTime));
}