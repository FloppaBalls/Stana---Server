#include "ClientList.h"


Client::Client(QTcpSocket* socket0 , qint16 id , QString key0) : socket(socket0) , userId(std::move(id)) , key(std::move(key0))
{}
void Client::next()
{
    if (responseQueue.empty() == false)
    {
        socket->write(responseQueue.front());
        socket->flush();
        responseQueue.pop();
    }
}
void Client::addResponse(QByteArray response)
{
    if (responseQueue.empty())
    {
        socket->write(std::move(response));
        socket->flush();
    }
    else
        responseQueue.emplace(std::move(response));
}
void Client::addResponses(std::vector<QByteArray> responses)
{
    if (responseQueue.empty())
    {
        for (auto b = responses.begin() + 1; b != responses.end(); b++)
            responseQueue.emplace(std::move(*b));

        socket->write(std::move(*responses.begin()));
        socket->flush();
    }
    else
    {
        for(QByteArray& r : responses)
            responseQueue.emplace(r);
    }
}


ClientList::ClientList(QObject* parent) : QObject(parent)
{
    loggedInClients = 0u;
}

void ClientList::addClient(const Client& client)
{
    if(client.userId == -1)
    {
        _clientHash.insert(Auxiliary::getClientKey(client.socket) , std::make_shared<Client>( client));
        qDebug() << "->New client connected - Status: not logged in";
    }
    else
    {
        std::shared_ptr<Client> c = _clientHash.insert(Auxiliary::getClientKey(client.socket) , std::make_shared<Client>(client)).value();
        //should I cast to void?
        insertClientInList(c);

        qDebug() << "->New client connected - Status: already logged in";
    }

}
void ClientList::addClient(const Client& client , qint16 id)
{
    std::shared_ptr<Client> c = std::make_shared<Client>(client);
    c->userId = std::move(id);
    _clientHash.insert(Auxiliary::getClientKey(client.socket) , insertClientInList(c));
    loggedInClients++;
    qDebug() << "->New client connected - Status: already logged in";

}

void ClientList::removeClient(const QString& key)
{
    Client& client = *_clientHash[key];
    _clientHash.remove(key);
    if(client.userId != -1)
    {
        auto beg = std::begin(_loggedClients);
        int pos = posById(client.userId);
        if(pos == -1)
        {
            qDebug() << " ! Could not delete the element because the Id was not found";
            qDebug() << " ! Ids in array: ";
            for(const auto& c : _loggedClients)
                qDebug() << c->userId;

            return;
        }
        _loggedClients.erase(beg + pos);
        qDebug() << "<-Client removed from connected list";
    }
    else
        client.socket->deleteLater();

}
Client& ClientList::clientByKey(const QString& key) { return *_clientHash[key];}
void ClientList::setIdForClient(const QString& clientKey , qint16 id)
{
    auto client = _clientHash[clientKey];
    if(posById(id) == -1)
    {
        client->userId = id;
        insertClientInList(client);
        qDebug() << "->New client logged in with id: " << id;
    }
    else
    {
        client->userId = id;
        qDebug() << "~~Client switched account with id: " << id;
    }
}


void ClientList::next(const QTcpSocket* socket) noexcept
{
    Client& client = clientByKey(Auxiliary::getClientKey(socket));
    client.next();
}
std::shared_ptr<Client> ClientList::insertClientInList(std::shared_ptr<Client> c)
{
    int mid , left , lastLeft , right , lastRight;
    lastLeft = left = 0;
    lastRight = right = _loggedClients.size() - 1;
    /* we are finding out , through binary search , the zone in which the client could be inserted so that we can keep the order*/
    while(left < right)
    {
        mid = (left + right) / 2;
        if(_loggedClients[mid]->userId == c->userId)
        {
            return *_loggedClients.insert(_loggedClients.begin() + mid , c);
        }
        else if(_loggedClients[mid]->userId > c->userId)
        {
            lastRight = right;
            lastLeft = left;
            right = mid - 1;
        }
        else if(_loggedClients[mid]->userId < c->userId)
        {
            lastLeft = left;
            lastRight = right;
            left = mid + 1;
        }
    }

    int end = std::min(lastRight + 1, (int)_loggedClients.size() - 1);
    int i = 0;
    for (i = std::max(lastLeft - 1 , 0) ; i <=  end; i++)
    {
        if (_loggedClients[i]->userId >= c->userId)
            return *_loggedClients.insert(_loggedClients.begin() + i, c);
    }

    return _loggedClients.emplace_back(c);
}

void ClientList::sendResponseToUsers(std::vector<int> userIdList , const QByteArray& message) noexcept
{
    for (const int& userId : userIdList)
        sendResponseToUser(userId, message);
}
void ClientList::sendResponseToUsers(std::vector<QByteArray> userIdList, const QByteArray& message) noexcept
{
    for (const QString& userId : userIdList)
        sendResponseToUser(userId.toInt(), message);
}

void ClientList::sendResponseToUser(int userId , const QByteArray& message) noexcept
{
    std::shared_ptr<Client> client = clientById(userId);
    if(client)
        client->addResponse(message);
}
void ClientList::sendResponseToUser(const QTcpSocket* socket, const QByteArray& response) noexcept
{
    Client& client = clientByKey(Auxiliary::getClientKey(socket));
    client.addResponse(response);
}
void ClientList::sendResponsesToUser(const QTcpSocket* socket, std::vector<QByteArray> responseList) noexcept
{
    Client& client = clientByKey(Auxiliary::getClientKey(socket));
    for (QByteArray& arr : responseList)
        client.addResponse(std::move(arr));
}
void ClientList::sendResponseToUsers(std::vector<int> userIdList , const QString& message) noexcept{   sendResponseToUsers(std::move(userIdList), message.toUtf8());}
void ClientList::sendResponseToUser(int userId , const QString& message) noexcept { sendResponseToUser(userId , message.toUtf8());}


std::shared_ptr<Client> ClientList::clientById(int id)
{
    int mid , left , right;
    left = 0;
    right = _loggedClients.size() - 1;
    /* we are finding out , through binary search , the zone in which the client could be inserted so that we can keep the order*/
    while(left <= right)
    {
        mid = (left + right) / 2;
        if(_loggedClients[mid]->userId == id)
        {
            return _loggedClients[mid];
        }
        else if(_loggedClients[mid]->userId > id)
        {
            right = mid - 1;
        }
        else if(_loggedClients[mid]->userId < id)
        {
            left = mid + 1;
        }
    }
    return nullptr;
}

int ClientList::posById(int id)
{
    int mid , left , right;
    left = 0;
    right = _loggedClients.size() - 1;
    /* we are finding out , through binary search , the zone in which the client could be inserted so that we can keep the order*/
    while(left <= right)
    {
        mid = (left + right) / 2;
        if(_loggedClients[mid]->userId == id)
        {
            return mid;
        }
        else if(_loggedClients[mid]->userId > id)
        {
            right = mid - 1;
        }
        else if(_loggedClients[mid]->userId < id)
        {
            left = mid + 1;
        }
    }
    return -1;
}

bool ClientList::clientActive(int id) const noexcept{
    int mid , left , right;
    left = 0;
    right = _loggedClients.size() - 1;
    /* we are finding out , through binary search , the zone in which the client could be inserted so that we can keep the order*/
    while(left <= right)
    {
        mid = (left + right) / 2;
        if(_loggedClients[mid]->userId == id)
        {
            return true;
        }
        else if(_loggedClients[mid]->userId > id)
        {
            right = mid - 1;
        }
        else if(_loggedClients[mid]->userId < id)
        {
            left = mid + 1;
        }
    }
    return false;
}

