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
#include "MediaHandler.h"
#include "MediaUploader.h"

/*Problems that I want to solve: 08/02/2024
    The fact that , when I send a message it is a problem that everytime one is sent in ANY chat , I have to always search for the participants in the database and
    search them in the connection list , if they are there. It is inneficient , especially if there are a lot of people using the app.

    I should create some sort of vector where I hold the ACTIVE chats (chatId) and the participants(QTcpSocket and userId) and whenever a message is sent from a chat ,
I first search if the chatId is in the active chat list. If it's not then I check if any of the participants to the chat are online. If there are , then I create an
active room in the vector.

    I think that , after about 5 min of no messaging , the active room should be deleted.
    Also , whenever someone disconnects , I have to remove them from all active chats.



*/

extern const QByteArray commandEnd;
extern const QByteArray commandBegin;


//for contact info
extern const QByteArray contactSeps[];

extern const QByteArray contactPrefix;

extern const QByteArray contact_idSep;
extern const QByteArray contact_nameSep;
extern const QByteArray contact_friendListSep;
extern const QByteArray contact_onlineSep;
extern const QByteArray contact_lastSeenSep;
extern const QByteArray contact_isBlockedSep;
extern const QByteArray contact_hasRequestSep;
//for message info
extern const QByteArray messagePrefix;

extern const QByteArray message_chatIdSep;
extern const QByteArray message_nameSep;
extern const QByteArray message_textSep;
extern const QByteArray message_timestampSep;
//for chat info
extern const QByteArray chatPrefix;

extern const QByteArray chat_idSep;
extern const QByteArray chat_nameSep;
extern const QByteArray chat_historySep;
extern const QByteArray chat_memberListSep;
extern const QByteArray chat_privateSep;
extern const QByteArray chat_isSenderSep;
extern const QByteArray chat_adminIdSep;
extern const QByteArray chat_readOnlyListSep;
extern const QByteArray chat_removedSep;
extern const QByteArray chat_newMembersSep;
//for user info
extern const QByteArray chunk_idSep;

extern const QByteArray userPrefix;

extern const QByteArray user_idSep;

class Message{
public:
    Message();
    QByteArray& operator[](int i);
private:
    std::vector<QByteArray> param;
};

class Server : public QObject{
    Q_OBJECT
public:
    explicit Server(QObject* parent = nullptr , int port = 1112);
private slots:
    void onNewConnection();
    void onReadyRead_Client();
    void onReadyRead_MediaProvider();
    void onConnectionToMediaProvider();
    void onNewMessage(const QByteArray& byteArr);
    void onDisconnected();
private:
    static std::vector<QByteArray> extractParameters(const QByteArray& arr);
    static std::vector<QByteArray> extractComplexParameters(const QByteArray& arr, RequestFromClient type);
    static std::vector<QByteArray> extractMediaChunkParameters(const QByteArray& arr);
    static int extractRequestNumber(const QByteArray& str);

    static std::vector<int> extractIntsFromArr(const QByteArray& str);

    QByteArray getSignInInfoForUser(int id);
    QByteArray getChatsForUser(int id);

    //this does NOT send the flags of a user
    QByteArray getContactsAndTheirInfoForUser(int id);

    std::vector<QByteArray> formAllDataOfUser(int userId);
    QByteArray formNewMessageInfo(int chatId , QByteArray sender , QByteArray text , QByteArray timestamp);
    //returns a list of users that have similar characters to this string
    QByteArray searchForUsers( const QByteArray& str , int clientId);
    std::vector<int> clientIdsForRoom( int roomId) const;

    //these two also SEND the flags of a user
    QByteArray contactDataOfUser(int id );
    QByteArray contactDataOfUsers(std::vector<int> idList );
    //
    void extractFromChat(const QByteArray& str);

    //extracts the information of a message from 'start' and returns a vector of the paramaters of a message in QByteArray
    //also return the end of the message
    std::pair<Message, int> message(const QByteArray& str , int start) const;

    QByteArray formMessageHistory(int chatId);
    void makeFriends(int id1 , int id2);
    void makeFriends(QByteArray id1 , QByteArray id2);
    void breakFriendship(QByteArray id1, QByteArray id2);
    void blockUserForUser(QByteArray id1, QByteArray id2);
    void unblockUserForUser(QByteArray id1, QByteArray id2);
    void removeFromGroup(QByteArray chatId, QByteArray userId , bool removed);
    void addPeopleToGroup(QByteArray chatId, std::vector<QByteArray> idList);
    //this functions should be called only between users who are friends , because this function doesn't send the data of users to each other
    //they already should have it because of their friendship
    //id1 is always the one who requests
    void sendNewChatToUsers(const QByteArray& id1, const QByteArray& id2 , const QByteArray& chatId);
    //the first id is the admin
    void sendNewChatToUsers(std::vector<QByteArray> list, const QByteArray& chatId);
    void changeChatName(const QByteArray& chatId, const QByteArray& newName);
    //returns id as QByteArray
    QByteArray createGroupChat(std::vector<QByteArray> list);
    QByteArray getContactInfoOfUsers(const QByteArray& id , std::vector<QByteArray> idlist);
    void sendServerAnnouncementToChat(QByteArray chatId , const QByteArray& text);
    void sendMessageToChatMembers(QByteArray chatId, const QByteArray& text);
    void sendMessageToFriendsOfUser(int userId , const QByteArray& text);
    void sendUploadIdToUser(QTcpSocket& socket , std::vector<QByteArray> args);
    void setStatusForUser(int id, bool online);

    void sendMediaToUsers(std::unique_ptr<MediaUploadingData> data , QByteArray fileName);
    void sendFileNameToSender(int userId , int handlingId, QByteArray name);
signals:
    void newMessage(const QByteArray& byteArr);
private:
    QTcpServer* pServer = nullptr;
    QTcpSocket* pMediaProviderSocket = nullptr;
    MediaHandler mediaHandler;
    MediaUploader mediaUploader;
    ClientList _list;

    std::unique_ptr<QSqlDatabase> _database;

    static constexpr qint16 searchReturnLimit = 30;
    static constexpr qsizetype chunkSize = 4096;
};

#endif // SERVER_H

