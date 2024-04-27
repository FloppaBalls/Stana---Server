#ifndef SERVER_H
#define SERVER_H
#include <QTcpServer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include "ClientList.h"
#include "Auxiliary.h"
#include <QDateTime>
/*Problems that I want to solve: 08/02/2024
    The fact that , when I send a message it is a problem that everytime one is sent in ANY chat , I have to always search for the participants in the database and
    search them in the connection list , if they are there. It is inneficient , especially if there are a lot of people using the app.

    I should create some sort of vector where I hold the ACTIVE chats (chatId) and the participants(QTcpSocket and userId) and whenever a message is sent from a chat ,
I first search if the chatId is in the active chat list. If it's not then I check if any of the participants to the chat are online. If there are , then I create an
active room in the vector.

    I think that , after about 5 min of no messaging , the active room should be deleted.
    Also , whenever someone disconnects , I have to remove them from all active chats.



*/

extern const QString commandEnd;
extern const QString commandBegin;


//for contact info
extern const QString contactSeps[];

extern const QString contactPrefix;

extern const QString contact_idSep;
extern const QString contact_nameSep;
extern const QString contact_friendListSep;
extern const QString contact_onlineSep;
extern const QString contact_lastSeenSep;
extern const QString contact_isBlockedSep;
extern const QString contact_hasRequestSep;
//for message info
extern const QString messagePrefix;

extern const QString message_chatIdSep;
extern const QString message_nameSep;
extern const QString message_textSep;
extern const QString message_timestampSep;
//for chat info
extern const QString chatPrefix;

extern const QString chat_idSep;
extern const QString chat_nameSep;
extern const QString chat_historySep;
extern const QString chat_memberListSep;
extern const QString chat_privateSep;
extern const QString chat_isSenderSep;
extern const QString chat_adminIdSep;
extern const QString chat_readOnlyListSep;
extern const QString chat_removedSep;
extern const QString chat_newMembersSep;
//for user info
extern const QString userPrefix;

extern const QString user_idSep;

class Message{
public:
    Message();
    QString& operator[](int i);
private:
    std::vector<QString> param;
};

class Server : public QObject{
    Q_OBJECT
public:
    explicit Server(QObject* parent = nullptr , int port = 1112);
private slots:
    void onNewConnection();
    void onReadyRead();
    void onNewMessage(const QByteArray& byteArr);
    void onDisconnected();
private:
    static std::vector<QString> extractParameters(const QString& str);
    static std::vector<QString> extractComplexParameters(const QString& str, RequestFromClient type);
    static RequestFromClient extractRequestType(const QString& str);
    static std::vector<int> extractIntsFromArr(const QString& str);

    QByteArray getSignInInfoForUser(int id);
    QByteArray getChatsForUser(int id);

    //this does NOT send the flags of a user
    QByteArray getContactsAndTheirInfoForUser(int id);

    QByteArray formAllDataOfUser(int userId);
    QByteArray formNewMessageInfo(int chatId , QString sender , QString text , QString timestamp);
    //returns a list of users that have similar characters to this string
    QByteArray searchForUsers( const QString& str , int clientId);
    std::vector<int> clientIdsForRoom( int roomId) const;

    //these two also SEND the flags of a user
    QByteArray contactDataOfUser(int id );
    QByteArray contactDataOfUsers(std::vector<int> idList );
    //
    void extractFromChat(const QString& str);

    //extracts the information of a message from 'start' and returns a vector of the paramaters of a message in qstring
    //also return the end of the message
    std::pair<Message, int> message(const QString& str , int start) const;
    Client& clientByKey(const QString& key);
    void removeByKey(const QString& key);

    QString formMessageHistory(int chatId);
    void makeFriends(int id1 , int id2);
    void makeFriends(QString id1 , QString id2);
    void breakFriendship(QString id1, QString id2);
    void blockUserForUser(QString id1, QString id2);
    void unblockUserForUser(QString id1, QString id2);
    void removeFromGroup(QString chatId, QString userId , bool removed);
    void addPeopleToGroup(QString chatId, std::vector<QString> idList);
    //this functions should be called only between users who are friends , because this function doesn't send the data of users to each other
    //they already should have it because of their friendship
    //id1 is always the one who requests
    void sendNewChatToUsers(const QString& id1, const QString& id2 , const QString& chatId);
    //the first id is the admin
    void sendNewChatToUsers(std::vector<QString> list, const QString& chatId);
    void changeChatName(const QString& chatId, const QString& newName);
    //returns id as QString
    QString createGroupChat(std::vector<QString> list);
    QByteArray getContactInfoOfUsers(const QString& id , std::vector<QString> idlist);
    void sendServerAnnouncementToChat(QString chatId , const QString& text);
    void sendMessageToChatMembers(QString chatId, const QString& text);
    void sendMessageToFriendsOfUser(int userId , const QString& text);

    void setStatusForUser(int id, bool online);
signals:
    void newMessage(const QByteArray& byteArr);
private:
    QTcpServer* pServer = nullptr;

    ClientList _list;
    std::unique_ptr<QSqlDatabase> _database;

    static constexpr qint16 searchReturnLimit = 30;
};

#endif // SERVER_H

