#include "Auxiliary.h"


int Converters::suffixType(const QString& suf)
{
    if (suf == "jpg")
        return (int)FileExtension::JPG;
    if (suf == "png")
        return (int)FileExtension::PNG;

    return (int)FileExtension::INVALID;

}
QString Auxiliary::getClientKey(const QTcpSocket* client)
{
    return client->peerAddress().toString().append(':').append(QByteArray::number(client->peerPort()));
}
QByteArray Auxiliary::quoteIt(const QByteArray& str)
{
    return '"' + str + '"';
}
QByteArray Auxiliary::safeQuoteIt(QByteArray str)
{
    return " \"" + str.replace('"', "\"") + " \"";
}

void Auxiliary::createCmd(QByteArray& info , InfoToClient type)
{
    info = commandBegin + "I" + QByteArray::number(int(type)) + ':' +  info + commandEnd;
}
void Auxiliary::createCmd(QByteArray& info, RequestFailure type)
{
    info = commandBegin + "F" + QByteArray::number(int(type)) + ':' + info + commandEnd;
}

QByteArray Auxiliary::makeList(std::vector<QByteArray> list, char sepBegin , char sepEnd)
{
    QByteArray str;
    str.append(sepBegin);
    for (int i = 0; i < list.size(); i++)
        str += list[i] + ',';
    str.back() = sepEnd;
    return str;
}

std::vector<int> Auxiliary::extractIntArray(const QByteArray& str)
{
    std::vector<int> memberList;
    QByteArray nrStr;
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

QDateTime Auxiliary::stringToDateTime(const QByteArray& str)
{
    int timeStampPos = 0;
    std::vector<int> date(3);
    std::vector<int> time(3);
    int i = 0;
    QByteArray aux;
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

int Auxiliary::suffixType(const QString& suf)
{
    if (suf == "jpg")
        return (int)FileExtension::JPG;
    if (suf == "png")
        return (int)FileExtension::PNG;

    return (int)FileExtension::INVALID;
}