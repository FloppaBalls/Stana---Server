
#ifndef AUXILIARY_H
#define AUXILIARY_H
#include <QTcpSocket>
#include <QByteArray>
#include <QDateTime>
#include <QTimeZone>

static const QByteArray commandEnd = "~\\";
static const QByteArray commandBegin = "~";


//for contact info
static const QByteArray contactSeps[] = { "\"OwnFriendList\":" , "\"RequestList\":" , "\"BlockedList\":" , "\"NecessaryList:\":" };

static const QByteArray contactPrefix = "con";

static const QByteArray contact_idSep = "\"" + contactPrefix + "Id\":";
static const QByteArray contact_nameSep = "\"" + contactPrefix + "Name\":";
static const QByteArray contact_friendListSep = "\"" + contactPrefix + "FriendList\":";
static const QByteArray contact_onlineSep = "\"" + contactPrefix + "Online\":";
static const QByteArray contact_lastSeenSep = "\"" + contactPrefix + "LastSeen\":";
static const QByteArray contact_isBlockedSep = "\"" + contactPrefix + "IsBlocked\":";
static const QByteArray contact_hasRequestSep = "\"" + contactPrefix + "HasRequest\":";
//for message info
static const QByteArray messagePrefix = "mes";

static const QByteArray message_chatIdSep = "\"" + messagePrefix + "ChatId\":";
static const QByteArray message_nameSep = "\"" + messagePrefix + "Name\":";
static const QByteArray message_textSep = "\"" + messagePrefix + "Text\":";
static const QByteArray message_timestampSep = "\"" + messagePrefix + "Timestamp\":";
//for chat info
static const QByteArray chatPrefix = "chat";

static const QByteArray chat_idSep = "\"" + chatPrefix + "Id\":";
static const QByteArray chat_nameSep = "\"" + chatPrefix + "Name\":";
static const QByteArray chat_historySep = "\"" + chatPrefix + "History\":";
static const QByteArray chat_memberListSep = "\"" + chatPrefix + "MemberList\":";
static const QByteArray chat_privateSep = "\"" + chatPrefix + "Private\":";
static const QByteArray chat_isSenderSep = "\"" + chatPrefix + "isSender\":";
static const QByteArray chat_adminIdSep = "\"" + chatPrefix + "Admin\":";
static const QByteArray chat_readOnlyListSep = "\"" + chatPrefix + "ReadOnlyList\":";
static const QByteArray chat_removedSep = "\"" + chatPrefix + "Removed\":";
static const QByteArray chat_newMembersSep = "\"" + chatPrefix + "NewMembers\":";

static const QByteArray chunk_idSep = "\"chunkId\":";
static const QByteArray chunk_acceptedSep = "\"chunkAccepted\":";
//for user info
static const QByteArray userPrefix = "user";

static const QByteArray user_idSep = "\"" + userPrefix + "Id\":";

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
    AddPeopleToTheChat,
    MediaUploadId,
    MediaChunk
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
    FriendStatus,
    UploadId,
    ChunkAccepted
};

enum class FileExtension {
    INVALID, PNG, JPG, COUNT
};

namespace Converters {
    int suffixType(const QString& suf);
}

class Auxiliary {
public:
    static QString getClientKey(const QTcpSocket* client);
    static QByteArray quoteIt(const QByteArray& str);
    //used for info that could contain the " character , it also replaces any " character with /"
    static QByteArray safeQuoteIt(QByteArray str);
    //it should look something like this: (str1 , str2 , str3 , .... , strn)
    static QByteArray makeList(std::vector<QByteArray> str , char sepBegin = '(', char sepEnd = ')');
    static void createCmd(QByteArray& info, RequestFailure type);
    static void createCmd(QByteArray& info, InfoToClient type);
    static std::vector<int> extractIntArray(const QByteArray& str);
    static QDateTime stringToDateTime(const QByteArray& str);
    static int suffixType(const QString& suf);
};
#endif // AUXILIARY_H
