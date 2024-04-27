
#ifndef AUXILIARY_H
#define AUXILIARY_H
#include <QTcpSocket>
#include <QString>
#include <QDateTime>
#include <QTimeZone>

static const QString commandEnd = "~\\";
static const QString commandBegin = "~";


//for contact info
static const QString contactSeps[] = { "\"OwnFriendList\":" , "\"RequestList\":" , "\"BlockedList\":" , "\"NecessaryList:\":" };

static const QString contactPrefix = "con";

static const QString contact_idSep = "\"" + contactPrefix + "Id\":";
static const QString contact_nameSep = "\"" + contactPrefix + "Name\":";
static const QString contact_friendListSep = "\"" + contactPrefix + "FriendList\":";
static const QString contact_onlineSep = "\"" + contactPrefix + "Online\":";
static const QString contact_lastSeenSep = "\"" + contactPrefix + "LastSeen\":";
static const QString contact_isBlockedSep = "\"" + contactPrefix + "IsBlocked\":";
static const QString contact_hasRequestSep = "\"" + contactPrefix + "HasRequest\":";
//for message info
static const QString messagePrefix = "mes";

static const QString message_chatIdSep = "\"" + messagePrefix + "ChatId\":";
static const QString message_nameSep = "\"" + messagePrefix + "Name\":";
static const QString message_textSep = "\"" + messagePrefix + "Text\":";
static const QString message_timestampSep = "\"" + messagePrefix + "Timestamp\":";
//for chat info
static const QString chatPrefix = "chat";

static const QString chat_idSep = "\"" + chatPrefix + "Id\":";
static const QString chat_nameSep = "\"" + chatPrefix + "Name\":";
static const QString chat_historySep = "\"" + chatPrefix + "History\":";
static const QString chat_memberListSep = "\"" + chatPrefix + "MemberList\":";
static const QString chat_privateSep = "\"" + chatPrefix + "Private\":";
static const QString chat_isSenderSep = "\"" + chatPrefix + "isSender\":";
static const QString chat_adminIdSep = "\"" + chatPrefix + "Admin\":";
static const QString chat_readOnlyListSep = "\"" + chatPrefix + "ReadOnlyList\":";
static const QString chat_removedSep = "\"" + chatPrefix + "Removed\":";
static const QString chat_newMembersSep = "\"" + chatPrefix + "NewMembers\":";
//for user info
static const QString userPrefix = "user";

static const QString user_idSep = "\"" + userPrefix + "Id\":";

enum RequestFromClient {
    Register,
    Login,
    AddMessage,
    SearchForUsers,
    ManageFriendRequest,  // commandNumber(userId , friendRequestUserId  , accepted)
    SendFriendRequest,
    CreatePrivateChat,
    RemoveFriend,
    BlockPerson,
    GetInfoOfUsers,
    UnblockUser,
    CreateGroupChat,
    UpdateChatName,
    RemoveFromGroup,
    AddPeopleToTheChat
};
enum RequestFailure {
    NameUsed,
    NameNotFound,
    IncorrectPassword,
    EmailUsed
};
enum InfoToClient {
    SignedIn,  // will inform the user that he has succesfully signed in
    ChatInfo, // will give the user the info of what chats he takes part in and it's messages
    Contacts, // will give the user his friend list , blocked people , people who are offline/online/last seen on yyyy-mm--dd
    NewMessage,
    SearchedUserList,
    NewFriend,   // will give the info of the new friend you just made
    NewFriendRequest,
    NewChat,
    FriendRemoved,
    PersonBlocked,
    YouGotBlocked,
    InfoOfUsers,
    UserUnblocked,
    UserUnblockedYou,
    NewChatName,
    GroupMemberRemoved,
    GroupMembersAdded,
    NecessaryContacts,
    NewAdmin,
    FriendStatus
};

class Auxiliary {
public:
    static QString getClientKey(const QTcpSocket* client);
    static QString quoteIt(const QString& str);
    //used for info that could contain the " character , it also replaces any " character with /"
    static QString safeQuoteIt(QString str);
    //it should look something like this: (str1 , str2 , str3 , .... , strn)
    static QString makeList(std::vector<QString> str , char sepBegin = '(', char sepEnd = ')');
    static void createCmd(QString& info, RequestFailure type);
    static void createCmd(QString& info, InfoToClient type);
    static std::vector<int> extractIntArray(const QString& str);
    static QDateTime stringToDateTime(const QString& str);
};
#endif // AUXILIARY_H
