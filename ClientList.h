
#ifndef CLIENTLIST_H
#define CLIENTLIST_H
#include <QTcpSocket>
#include <QHash>
#include "Auxiliary.h"
#include <queue>

struct Client{
    Client(QTcpSocket* socket0 , qint16 id , QString key);

    void next();
    void addResponse(QByteArray response);
    void addResponses(std::vector<QByteArray> responses);

    QTcpSocket* socket;
    qint16 userId = -1;
    QString key;
    std::queue<QByteArray> responseQueue;
};

class ClientList : public QObject{
public:
    ClientList(QObject* parent);
    void addClient(const Client& client);
    //this is when someone has the app and wants to be kept logged in at launch
    void addClient(const Client& client , qint16 id);
    void removeClient(const QString& key);
    Client& clientByKey(const QString& key);
    void setIdForClient(const QString& clientKey , qint16 id);

    void sendResponseToUsers(std::vector<int> userIdList , const QString& message) noexcept;
    void sendResponseToUsers(std::vector<int> userIdList , const QByteArray& message) noexcept;
    void sendResponseToUsers(std::vector<QByteArray> userIdList, const QByteArray& message) noexcept;
    void sendResponseToUser(const QTcpSocket* socket, const QByteArray& response) noexcept;
    void sendResponsesToUser(const QTcpSocket* socket, std::vector<QByteArray> responseList) noexcept;
    void sendResponseToUser(int userId , const QString& message) noexcept;
    void sendResponseToUser(int userId , const QByteArray& message) noexcept;
    void next(const QTcpSocket* socket) noexcept;

    bool clientActive(int userId) const noexcept;
    std::shared_ptr<Client> clientById(int id);
private:
    std::shared_ptr<Client> insertClientInList(std::shared_ptr<Client> c);
    //applies binary search
    int                     posById(int id);
private:
    quint16 loggedInClients;
    //this is an array of clients sorted by the user id , for quick search of a user by id
    //this vector holds all the logged clients in the hash table , while the hash table holds the logged and the unlogged ones
    std::vector<std::shared_ptr<Client>> _loggedClients;
    /*this is a hashtable of clients for searching a user by it's key , slower , but not as slow as searching in a linear way
     every single socket for the correct key */
    QHash<QString , std::shared_ptr<Client>> _clientHash;
};



#endif // CLIENTLIST_H
