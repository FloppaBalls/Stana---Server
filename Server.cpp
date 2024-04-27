#include "Server.h"
#include "Auxiliary.h"

using Aux = Auxiliary;


Message::Message()
{
    param.resize(3);
}
QString& Message::operator[](int i) { return param[i];}


Server::Server(QObject* obj , int port)
    :QObject(obj) , _list(this)
{
    pServer = new QTcpServer(this);

    if(!pServer->listen(QHostAddress::LocalHost , port))
    {
        qDebug() << "You fumbled the server";
        qDebug() << pServer->errorString();
    }
    else
    {
        qDebug() << pServer->serverAddress();
        qDebug() << "Listening...";
    }

    qDebug() << "Port:" << pServer->serverPort();
    connect(pServer, SIGNAL(newConnection()) , this , SLOT(onNewConnection()));
    connect(this , &Server::newMessage , this , &Server::onNewMessage);

    QString _databaseName = "sheeplydatabase",
            _loginUsername = "tcpserver",
            _loginPassword = "chungus",
            _databaseIp = "localhost";
    int     _loginPort = 5432;

    _database = std::make_unique<QSqlDatabase>(QSqlDatabase::addDatabase("QPSQL"));
    _database->setPort(_loginPort);
    _database->setHostName(_databaseIp);
    _database->setUserName(_loginUsername);
    _database->setPassword(_loginPassword);
    _database->setDatabaseName(_databaseName);

    if(_database->open())
    {
        qDebug() << "Succesfully opened the database";
    }
    else
        qDebug() << "Failed to open the database";

    //QSqlQuery query(*_database);
    //query.exec("SELECT history FROM rooms;");
    //while(query.next())
    //{
    //    //extractFromChat(query.value(0).toString());
    //    extractFromChat(query.value(0).toString());
    //}

    //qDebug() << getChatsForUser(1);

}


void Server::extractFromChat(const QString& str)
{
    /* example : {"(\"Andrei alexander\",\"Ha ba la baie\",\"2021-03-05 11:34:05\")","(Leo,\"Ce te pisi atata\",\"2021-04-05 00:00:00\")"}
     * "\"" marks the beginning of a text while "\"" marks the end
     * However not all texts start like this , only those who contain a space
     * */
    int pos = 0;
    int lastPos =  pos;
    std::vector<Message> history;
    while(lastPos < pos)
    {
        auto result = message(str , pos);
        lastPos = pos;
        pos = result.second;
        history.emplace_back(result.first);
    }
}

//it will return an array of string: sender name , the text itself and the time that it has been to
std::pair<Message, int> Server::message(const QString& str , int start) const
{
    /* example of a message: "(\"Andrei alexander\",\"Ha ba la baie\",\"2021-03-05 11:34:05\")"
     * */
    int pos = start;
    const QString paramSeparator = "\\\"";

    const short sepLength = paramSeparator.length();
    Message m;

    for(char i = 0 ; i < 3 ; i++)
    {
        qsizetype paramBeg = str.indexOf(paramSeparator , pos);
        qsizetype paramEnd = str.indexOf(paramSeparator , paramBeg + sepLength);
        m[i] = (str.mid(paramBeg + sepLength , paramEnd - paramBeg - sepLength));

        pos = paramEnd + sepLength;
    }
    qDebug() << "User: " << m[0] <<"---" << "Text: " << m[1] <<"---"<< "Timestamp: " << m[2];

    return {std::move(m) , str.indexOf('(' , pos)};
}

void Server::onNewConnection()
{
    QTcpSocket* pSocket = pServer->nextPendingConnection();
    if(pSocket == nullptr) return;

    qDebug() << "New client connected";
    _list.addClient(Client(pSocket , -1 , Aux::getClientKey(pSocket)));

    connect(pSocket , &QTcpSocket::readyRead , this , &Server::onReadyRead);
    connect(pSocket , &QTcpSocket::disconnected , this , &Server::onDisconnected);
}

void Server::onDisconnected()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (client == nullptr) return;

    QString key = Aux::getClientKey(client);

    Client& clientInfo = _list.clientByKey(key);
    if(clientInfo.userId != -1)
    setStatusForUser(clientInfo.userId, false);
    _list.removeClient(key);
}

void Server::onReadyRead()
{
    const auto client = qobject_cast<QTcpSocket*>(sender());
    if(client == nullptr) return;

    const auto message = client->readAll();
    qDebug() << "Request: " + message;

    QString str(message);

    RequestFromClient cmdType = extractRequestType(str);
    QSqlQuery query(*_database);

    std::vector<QString> paramList;
    if (cmdType != RequestFromClient::AddMessage && cmdType != RequestFromClient::UpdateChatName)
        paramList = extractParameters(str);
    else
        paramList = extractComplexParameters(str , cmdType);

    QString aux;
    QByteArray byteArr;
    switch(cmdType)
    {
    case RequestFromClient::Register:
        //verify if there is somebody already with this username or email

        if(query.exec("SELECT * FROM appusers WHERE name = '" + paramList[0] + "';") == false)
            qDebug() << "Find existing name query failed: " + query.lastError().text();

        if(query.next())
        {
            aux = commandBegin + "F" + QString::number(int(RequestFailure::NameUsed));
            client->write(aux.toUtf8());
            break;
        }
        if(query.exec("SELECT * FROM appusers WHERE email = '" + paramList[1] + "';") == false)
            qDebug() << "Find existing email query failed: " + query.lastError().text();
        if(query.next())
        {
            aux = commandBegin +"F" + QString::number(int(RequestFailure::EmailUsed));
            client->write(aux.toUtf8());
            break;
        }

        aux = "INSERT INTO appusers (name , email , password , isonline) VALUES ('%1' , '%2' , '%3','true') RETURNING id;";
        aux = aux.arg(paramList[0]).arg(paramList[1]).arg(paramList[2]);
        qDebug() << aux;
        query.exec(aux);
        _list.setIdForClient(Aux::getClientKey(client), aux.toInt());

        //aux = "I" + QString::number(int(InfoToClient::SignedIn));
        //client->write(aux.toUtf8());

        client->write(formAllDataOfUser(query.value(0).toInt()));
        client->flush();
        break;
    case RequestFromClient::Login:
        // check if it is a valid username

        if(query.exec("SELECT * FROM appusers WHERE name = '" + paramList[0] + "';") == false)
            qDebug() << "Find existing name query failed: " + query.lastError().text();

        if(query.next())
        {
            //password is the forth column in appusers table

            aux = query.value(3).toString();
            //the third parameter should be the password
            if(aux == paramList[1])
            {
                aux = formAllDataOfUser(query.value(0).toInt());
                _list.setIdForClient(Aux::getClientKey(client) , query.value(0).toInt());
            }
            else
                aux = commandBegin + "F" + QString::number(int(RequestFailure::IncorrectPassword)) + ';';
        }
        else
            aux = "~F" + QString::number(int(RequestFailure::NameNotFound)) + ';';


        setStatusForUser(query.value(0).toInt(), true);
        client->write(aux.toUtf8());
        client->flush();
        break;
    case RequestFromClient::AddMessage:
        qDebug() << "Sending message...";

        //ex UPDATE rooms SET history = array_append(history , ROW('Daquaivius' , 'nah like fr fr fr' , '2033-04-05 00:00:00')::message) WHERE id = 10;
        aux = "UPDATE rooms SET history = array_append(history , ROW('%1' , '%2' , '%3')::message) WHERE id = " + paramList[0] + ';';
        aux = aux.arg(paramList.at(1)).arg(paramList.at(2)).arg(paramList.at(3));
        query.prepare(aux);


        if(query.exec() == false)
            qDebug() << query.lastError();

        byteArr = formNewMessageInfo(paramList.at(0).toInt() , paramList.at(1) , paramList.at(2) , paramList.at(3));
        _list.sendMessageToUsers(clientIdsForRoom(paramList.at(0).toInt()) , byteArr );
        break;
    case RequestFromClient::SearchForUsers:
        aux = commandBegin + "I" + QString::number(int(InfoToClient::SearchedUserList)) + ':' + searchForUsers(paramList[1] , paramList[0].toInt()) + commandEnd;
        client->write(aux.toUtf8());
        client->flush();
        break;
    case RequestFromClient::ManageFriendRequest:
        // commandNumber(userId , friendRequestUserId  , accepted)
        query.exec("UPDATE appusers SET requests = ARRAY_REMOVE(requests , " + paramList[1] + ") WHERE id = " + paramList[0] + ';');
        if(paramList[2].toInt())
        {
            makeFriends(paramList[0] , paramList[1]);
        }
        break;
    case RequestFromClient::SendFriendRequest:
        query.exec("UPDATE appusers SET requests = ARRAY_APPEND(requests , " + paramList[0] + ") WHERE id = " + paramList[1] + ';');
        aux = commandBegin + "I" + QString::number(int(InfoToClient::NewFriendRequest)) + ':' + contactDataOfUser(paramList[0].toInt()) + commandEnd;
        _list.sendMessageToUser(paramList[1].toInt() , aux);
        break;
    case RequestFromClient::CreatePrivateChat:
        //example: 
        //INSERT INTO rooms (name , history , memberlist , private) VALUES ('Private' , ARRAY[('Server' , 'Come on , break the ice' , CURRENT_TIMESTAMP)]::message[] , 
        //ARRAY[4 , 5] , 'true') RETURNING id;
        aux = "INSERT INTO rooms (name , history , memberlist , private) VALUES" 
            "('Private' , ARRAY[('Server' , 'Come on , break the ice' , CURRENT_TIMESTAMP)]::message[] , ARRAY[%1 , %2] , 'true') RETURNING id;";
        aux = aux.arg(paramList[0]).arg(paramList[1]);
        query.exec(aux);
        query.next();
        sendNewChatToUsers(paramList[0], paramList[1] , query.value(0).toString());
        break;
    case RequestFromClient::RemoveFriend:
        breakFriendship(paramList[0], paramList[1]);
        break;
    case RequestFromClient::BlockPerson:
        blockUserForUser(paramList[0], paramList[1]);
        break;
    case RequestFromClient::GetInfoOfUsers:
        aux = paramList.back();
        paramList.pop_back();
        client->write(getContactInfoOfUsers(aux, paramList));
        client->flush();
        break;
    case RequestFromClient::UnblockUser:
        unblockUserForUser(paramList[0], paramList[1]);
        break;
    case RequestFromClient::CreateGroupChat:
        aux = createGroupChat(paramList);
        sendNewChatToUsers(paramList, aux);
        break;
    case RequestFromClient::UpdateChatName:
        changeChatName(paramList[0], paramList[1]);
        break;
    case RequestFromClient::RemoveFromGroup:
        removeFromGroup(paramList[0], paramList[1] , paramList[2].front() == 't');
        break;
    case RequestFromClient::AddPeopleToTheChat:
        //the chat id
        aux = paramList.back();
        paramList.pop_back();
        addPeopleToGroup(aux, paramList);
        break;
    default:
        break;
    }
}

void Server::addPeopleToGroup(QString chatId, std::vector<QString> idList)
{
    QString cmd1 = chat_idSep + Aux::quoteIt(chatId) + chat_newMembersSep + Aux::quoteIt(Aux::makeList(idList, '{', '}'));
    Aux::createCmd(cmd1, InfoToClient::GroupMembersAdded);

    sendMessageToChatMembers(chatId, cmd1);

    QSqlQuery query(*_database);
    cmd1 = "UPDATE rooms SET memberlist = memberlist || ARRAY" + Aux::makeList(idList, '[', ']') + " WHERE id = " + chatId + ';';
    query.exec(cmd1);


    sendNewChatToUsers(idList, chatId);

    for (QString strId : idList)
    {
        query.exec("SELECT name FROM appusers WHERE id = " + strId + ';');
        query.next();

        sendServerAnnouncementToChat(chatId, query.value(0).toString() + " was added to the group");
    }
}

void Server::sendMessageToChatMembers(QString chatId, const QString& text)
{
    QSqlQuery query(*_database);
    query.exec("SELECT memberlist FROM rooms WHERE id = " + chatId + ';');
    query.next();

    std::vector<int> list = Aux::extractIntArray(query.value(0).toString());
    _list.sendMessageToUsers(list, text);
}

void Server::removeFromGroup(QString chatId, QString userId, bool removed)
{
    QSqlQuery query(*_database);
    query.exec("SELECT memberlist FROM rooms WHERE id = " + chatId + ';');
    query.next();

    std::vector<int> list = Aux::extractIntArray(query.value(0).toString());
    //removing the id from the memberlist
    query.exec("UPDATE rooms SET memberlist = array_remove( memberlist , " + userId + " ) WHERE id = " + chatId + " RETURNING admin;");
    query.next();
    // retaining the admin id for later
    int adminId = query.value(0).toInt();
    //sending to the group members the fact that the member was removed 
    QString cmd = chat_idSep + Aux::quoteIt(chatId) + contact_idSep + Aux::quoteIt(userId) + chat_removedSep +
            Aux::quoteIt((removed) ? "true" : "false");

    Aux::createCmd(cmd, InfoToClient::GroupMemberRemoved);
    query.exec("SELECT name FROM appusers WHERE id = " + userId + ';');
    query.next();
    sendServerAnnouncementToChat(chatId, query.value(0).toString() +
        ((removed) ? " was removed from the group" : " left the group"));

    _list.sendMessageToUsers(list, cmd);

    //checking if the admin is the one who was removed
    bool newAdmin = false;
    if (adminId == userId.toInt())
    {
        query.exec("UPDATE rooms SET admin = memberlist[1] WHERE id = " + chatId + " RETURNING admin , (SELECT name FROM appusers WHERE id = admin);");
        newAdmin = true;
    }
    //sending to the group members the new admin 
    if (newAdmin)
    {
        query.next();
        cmd = chat_idSep + Aux::quoteIt(chatId) + contact_idSep + Aux::quoteIt(query.value(0).toString());
        Aux::createCmd(cmd, InfoToClient::NewAdmin);

        _list.sendMessageToUsers(list, cmd);

        sendServerAnnouncementToChat(chatId, query.value(1).toString() + " is the new Admin");
    }
}

void Server::unblockUserForUser(QString id1, QString id2)
{
    QSqlQuery query(*_database);
    query.exec("UPDATE appusers SET blocked = ARRAY_REMOVE(blocked ," + id2 + ") WHERE id = " + id1 + ';');

    auto client1 = _list.clientById(id1.toInt());
    if (client1.get())
    {
        QString str = contact_idSep + Aux::quoteIt(id2);
        Aux::createCmd(str, InfoToClient::UserUnblocked);
        client1->socket->write(str.toUtf8());
        client1->socket->flush();
    }

    auto client2 = _list.clientById(id2.toInt());
    if (client2.get())
    {
        QString str = contact_idSep + Aux::quoteIt(id1);
        Aux::createCmd(str, InfoToClient::UserUnblockedYou);
        client2->socket->write(str.toUtf8());
        client2->socket->flush();
    }
}

QByteArray Server::getContactInfoOfUsers(const QString& id , std::vector<QString> idList)
{
    QString cmd = "SELECT id , name , friends , isonline , lastseen , (blocked @> ARRAY[%1]) ," 
        "(requests @> ARRAY[%2]) as hasSenRequest  FROM appusers WHERE id IN " + Auxiliary::makeList(idList) + ';';

    cmd = cmd.arg(id).arg(id);
    QSqlQuery query(*_database);
    if (query.exec(cmd) == false)
        qDebug() << "Nigga";

    //"SELECT id , name , friends , isonline , lastseen , (blocked @> ARRAY[%1]) as isblocked , "
    //"(requests @> ARRAY[%2]) as hasSentRequest FROM appusers WHERE name ILIKE '%" + str + "%';";
    QString result = "{";
    while (query.next())
    {
        result += contact_idSep + '"' + query.value(0).toString() + '"';
        result += contact_nameSep + '"' + query.value(1).toString() + '"';
        result += contact_friendListSep + '"' + query.value(2).toString() + '"';
        result += contact_onlineSep + '"' + query.value(3).toString() + '"';
        result += contact_lastSeenSep + '"' + query.value(4).toString() + '"';
        result += contact_isBlockedSep + '"' + query.value(5).toString() + '"';
        result += contact_hasRequestSep + '"' + query.value(6).toString() + '"';
    }
    result += '}';
    Auxiliary::createCmd(result, InfoToClient::InfoOfUsers);
    return result.toUtf8();
}

void Server::blockUserForUser(QString id1, QString id2)
{
    QSqlQuery query(*_database);
    query.exec("UPDATE appusers SET friends = ARRAY_REMOVE(friends ," + id1 + ") WHERE id = " + id2 + ';');
    query.exec("UPDATE appusers SET friends = ARRAY_REMOVE(friends ," + id2 + ") WHERE id = " + id1 + ';');
    query.exec("UPDATE appusers SET blocked = ARRAY_APPEND(blocked ," + id2 + ") WHERE id = " + id1 + ';');
    query.exec("UPDATE appusers SET requests = ARRAY_REMOVE(requests ," + id1 + ") WHERE id = " + id2 + ';');

    auto client1 = _list.clientById(id1.toInt());
    if (client1.get())
    {
        QString str = contact_idSep + Aux::quoteIt(id2);
        Aux::createCmd(str, InfoToClient::PersonBlocked);
        client1->socket->write(str.toUtf8());
        client1->socket->flush();
    }

    auto client2 = _list.clientById(id2.toInt());
    if (client2.get())
    {
        QString str = contact_idSep + Aux::quoteIt(id1);
        Aux::createCmd(str, InfoToClient::YouGotBlocked);
        client2->socket->write(str.toUtf8());
        client2->socket->flush();
    }
}
void Server::breakFriendship(QString id1, QString id2)
{
    QSqlQuery query(*_database);
    query.exec("UPDATE appusers SET friends = ARRAY_REMOVE(friends ," + id1 + ") WHERE id = " + id2 + ';');
    query.exec("UPDATE appusers SET friends = ARRAY_REMOVE(friends ," + id2 + ") WHERE id = " + id1 + ';');

    auto client1 = _list.clientById(id1.toInt());
    if (client1.get())
    {
        QString str = contact_idSep + Aux::quoteIt(id2);
        Aux::createCmd(str, InfoToClient::FriendRemoved);
        client1->socket->write(str.toUtf8());
        client1->socket->flush();
    }

    auto client2 = _list.clientById(id2.toInt());
    if (client2.get())
    {
        QString str = contact_idSep + Aux::quoteIt(id2);
        Aux::createCmd(str, InfoToClient::FriendRemoved);
        client2->socket->write(str.toUtf8());
        client2->socket->flush();
    }
}

void Server::makeFriends(int id1 , int id2)
{
    makeFriends(QString::number(id1) ,QString::number(id2));
}
void Server::makeFriends(QString id1 , QString id2)
{
    QSqlQuery query(*_database);
    query.exec("UPDATE appusers SET friends = ARRAY_APPEND(friends ," + id1 + ") WHERE id = " + id2 + ';');
    query.exec("UPDATE appusers SET friends = ARRAY_APPEND(friends ," + id2 + ") WHERE id = " + id1 + ';');


    /*      result += nameSep + '"' + userQuery.value(0).toString() +'"';
            result += onlineSep + '"' + userQuery.value(1).toString() +'"';
            result += lastSeenSep + '"' + userQuery.value(2).toString() +'"';\
            result += friendListSep + '"' + userQuery.value(3).toString() +'"';*/
    auto client1 = _list.clientById(id1.toInt());
    QString text;
    if(client1.get())
    {
        query.exec("SELECT name , isonline , lastseen , friends from appusers WHERE id = " + id2 + ';');
        query.next();
        text = commandBegin + 'I' + QString::number(int(InfoToClient::NewFriend)) +':';
        text += contact_idSep + '"' + id2 +'"' +
               contact_nameSep + '"' + query.value(0).toString() +'"' +
               contact_onlineSep + '"' + query.value(1).toString() +'"' +
               contact_lastSeenSep + '"' + query.value(2).toString() +'"' +
               contact_friendListSep + '"' + query.value(3).toString() +'"';
        text += commandEnd;
        client1->socket->write(text.toUtf8());
        client1->socket->flush();
    }

    auto client2 = _list.clientById(id2.toInt());
    if(client2.get())
    {
        query.exec("SELECT name , isonline  , lastseen , friends from appusers WHERE id = " + id1 + ';');
        query.next();

        text = commandBegin + 'I' + QString::number(int(InfoToClient::NewFriend)) +':';
        text += contact_idSep + '"' + id1 +'"' +
               contact_nameSep + '"' + query.value(0).toString() +'"' +
               contact_onlineSep + '"' + query.value(1).toString() +'"' +
               contact_lastSeenSep + '"' + query.value(2).toString() +'"' +
               contact_friendListSep + '"' + query.value(3).toString() +'"';
        text += commandEnd;
        client2->socket->write(text.toUtf8());
        client2->socket->flush();
    }
}

/* Ma gandeam sa trimit data-ul la client in bucati dar aparent data ul acela poate sa se combine si asta ar afecta citirea lui de catre client */
QByteArray Server::formAllDataOfUser( int userId)
{
    /* data format:
     *  All comands/requests or whatever will start with the character '~' and end with '\~'
     *  Why?
     *  Because the client , when the readyRead signal activates , the data that should read could be a merge of 2 or more data transfers
     *  To combat this , even if the data is merged , '~' and ';' will separate them and when the client reads server data , it should loop
     *  over every time it encounters the '~' character
     *  ex: ~I0;
     *      ~I1 CHAT{ name , { {senderName , text , yyyy-mm-dd hh:mm:ss} , {} , ....} , {memberId0 , memberId1 , ... , memberIdN} },CHAT{},....\~
     *      ~I2 FRIENDS{ {id0 , online(bool) , lastSeen(timestamp) , } , id1 , id2 ,..... ,idn), FRIEND-REQUESTS { id3 , id5 ....} , BLOCKED { id1 , id2 , id3 ,...}\~
     * */
    QString info = QString( commandBegin + "I0:" + getSignInInfoForUser(userId) + commandEnd) +
                   commandBegin + "I1:" + getChatsForUser(userId) + commandEnd +
                   commandBegin + "I2:" + getContactsAndTheirInfoForUser(userId) + commandEnd;
    return info.toUtf8();
}
QByteArray Server::getSignInInfoForUser(int id)
{
    QString str = user_idSep + '"' + QString::number(id) + '"';
    return str.toUtf8();
}

QByteArray Server::getChatsForUser(int id)
{
    QSqlQuery query(*_database);
    query.prepare("SELECT id , name , private , memberlist , admin , read_only_members FROM rooms WHERE :id = ANY(memberlist);");
    query.bindValue(":id" , id);
    query.exec();

    QString chats;

    while(query.next())
    {
        chats += chat_idSep           + Aux::quoteIt(query.value(0).toString());
        chats += chat_nameSep         + Aux::quoteIt(query.value(1).toString());
        chats += chat_historySep      + Aux::quoteIt(formMessageHistory(query.value(0).toInt()));
        chats += chat_privateSep      + Aux::quoteIt(query.value(2).toString());
        chats += chat_memberListSep   + Aux::quoteIt(query.value(3).toString());
        //extended
        chats += chat_adminIdSep      + Aux::quoteIt(query.value(4).toString());
        chats += chat_readOnlyListSep + Aux::quoteIt(query.value(5).toString());
    }
    return chats.toUtf8();
}

QString Server::formMessageHistory(int chatId)
{
    QSqlQuery query(*_database);
    query.prepare("SELECT (unnest(r.history)).* FROM rooms r WHERE id = :id;");
    query.bindValue(":id" , chatId);
    query.exec();

    QString str = "{";
    while(query.next())
    {
        str += '(';
        str += message_nameSep      + query.value(0).toString();
        str += message_textSep      + query.value(1).toString();
        str += message_timestampSep + query.value(2).toString().replace('T' , ' ');
        str += ')';
    }
    str += '}';
    return str;
}

QByteArray Server::formNewMessageInfo(int chatId , QString sender , QString text , QString timestamp)
{
    QString str (commandBegin + 'I' + QString::number((int)InfoToClient::NewMessage) + ':' +
                message_chatIdSep + "\"" + QString::number(chatId) + "\"" + message_nameSep + "\"" + sender + "\"" + 
                message_textSep + Aux::quoteIt(text.replace('"' , "\\\"")) +
                message_timestampSep + "\"" + timestamp + "\"" + commandEnd);

    return str.toUtf8();
}

QByteArray Server::getContactsAndTheirInfoForUser( int id)
{
    QSqlQuery query(*_database);
    query.exec("SELECT friends , requests , blocked FROM appusers WHERE id = " + QString::number(id));
    query.next();

    QString result;

    auto replace = [](QString& str){
        str = str.replace('{' , '(').replace('}' , ')');
    };
    std::vector<QString> lists(3);
    for(auto i = 0 ; i < lists.size() ;i++)
    {
        result += contactSeps[i];
        lists[i] = query.value(i).toString();
        replace(lists[i]);
        QSqlQuery userQuery;
        userQuery.exec("SELECT id , name , isonline , lastseen , friends , requests from appusers WHERE id IN " + lists[i] + ';');

        while(userQuery.next())
        {
            result += contact_idSep + '"' + userQuery.value(0).toString() +'"';
            result += contact_nameSep + '"' + userQuery.value(1).toString() +'"';
            result += contact_onlineSep + '"' + userQuery.value(2).toString() +'"';
            result += contact_lastSeenSep + '"' + userQuery.value(3).toString() +'"';\
            result += contact_friendListSep + '"' + userQuery.value(4).toString() +'"';
        }
    }
    qDebug() << result;
    return result.toUtf8();
}
std::vector<QString> Server::extractParameters(const QString& str)
{
    int paramBeg = str.indexOf('(');
    int paramEnd = str.indexOf(')');

    if(paramEnd == -1 || paramBeg == -1)
        qDebug() << "Error: Did not provide parantheses for parameters";

    std::vector<QString> parameterList;
    QString aux;
    for(int i = paramBeg + 1 ;  i < paramEnd ; i++)
    {
        QChar ch = str[i];
        if(ch == ',')
        {
            parameterList.emplace_back(aux);
            aux = "";
        }
        else
            aux += ch;
    }
    parameterList.emplace_back(aux);

    return parameterList;
}

std::vector<QString> Server::extractComplexParameters(const QString& str, RequestFromClient type)
{
    const QString paramMessageBegStr = " (";
    const QString paramMessageEndStr = " )";
    const QString paramSep = " ,";
    int paramBeg = str.indexOf(paramMessageBegStr);
    int paramEnd = str.indexOf(paramMessageEndStr);

    if (paramEnd == -1 || paramBeg == -1)
        qDebug() << "Error: Did not provide parantheses for parameters";

    std::vector<QString> parameterList;
    //for (int i = paramBeg + 1; i < paramEnd; i++)
    //{
    //    QChar ch = str[i];
    //    if (ch == ',')
    //    {
    //        parameterList.emplace_back(aux);
    //        aux = "";
    //    }
    //    else
    //        aux += ch;
    //}
   
    int lastIndex = paramBeg + paramMessageBegStr.length();
    int index = str.indexOf(paramSep, lastIndex);

    while (index != -1)
    {
        index += paramSep.length();
        parameterList.emplace_back(str.mid(lastIndex, index - lastIndex - paramSep.length()));

        lastIndex = index;
        index = str.indexOf(paramSep, lastIndex);
    }
    parameterList.emplace_back(str.mid(lastIndex, str.length() - lastIndex - 2));
    //removing the last two characters of the last parameter
    if(type == RequestFromClient::AddMessage)
        parameterList[2] = parameterList[2].replace("\\(", "(").replace("\\)" , ")").replace("\\," , ",");
    else if(type == RequestFromClient::UpdateChatName)
        parameterList[1] = parameterList[1].replace("\\(", "(").replace("\\)", ")").replace("\\,", ",");

    return parameterList;
}
RequestFromClient Server::extractRequestType(const QString& str)
{
    //begginging of paramaters
    int paramBeg = str.indexOf('(') ;
    //string form of the command number
    QString cmdStr;
    for(int i = 0 ; i < paramBeg ; i++)
        cmdStr.append(str.at(i));

    qDebug() << "Extracted command: " + cmdStr;

    return RequestFromClient(cmdStr.toInt());
}
void Server::onNewMessage(const QByteArray& byteArr)
{
    //for(const auto& client : std::as_const(_clients))
    //{
    //    client.socket->write(byteArr);
    //    client.socket->flush();
    //}
}

std::vector<int> Server::clientIdsForRoom(int roomId) const
{
    QSqlQuery query(*_database);
    query.exec("SELECT memberlist FROM rooms WHERE id = " + QString::number(roomId) + ';');
    query.next();
    QString str = query.value(0).toString();

    std::vector<int> memberList(extractIntsFromArr(str));
    return memberList;
}

std::vector<int> Server::extractIntsFromArr(const QString& str)
{
    std::vector<int> memberList;
    QString nrStr;
    qint16 i = str.indexOf('{') + 1;

    while(str[i] != '}')
    {
        if(str[i] == ',')
        {
            memberList.emplace_back(nrStr.toInt());
            nrStr ="";
        }
        else
            nrStr += str[i];
        i++;
    }
    memberList.emplace_back(nrStr.toInt());
    return memberList;
}

QByteArray Server::searchForUsers(const QString& str , int clientId)
{
    QSqlQuery query(*_database);
    QString clientStr = QString::number(clientId);
    QString command = "SELECT id , name , friends , isonline , lastseen , (blocked @> ARRAY[%1]) as isblocked , "
                      "(requests @> ARRAY[%2]) as hasSentRequest FROM appusers WHERE name ILIKE '%" + str + "%';";

    command = command.arg(clientStr).arg(clientStr);
    query.prepare(command);

    query.exec();
    QString result;

    while(query.next())
    {
        //if(clientId == query.value(0).toInt()) continue;

        result += '{';
        result += contact_idSep         + '"' + query.value(0).toString() + '"';
        result += contact_nameSep       + '"' + query.value(1).toString() + '"';
        result += contact_friendListSep + '"' + query.value(2).toString() + '"';
        result += contact_onlineSep     + '"' + query.value(3).toString() + '"';
        result += contact_lastSeenSep   + '"' + query.value(4).toString() + '"';
        result += contact_isBlockedSep  + '"' + query.value(5).toString() + '"';
        result += contact_hasRequestSep + '"' + query.value(6).toString() + '"';
        result += '}';
    }

    return result.toUtf8();
}

QByteArray Server::contactDataOfUsers(std::vector<int> idList )
{
    QString cmd;
    for (int id : idList)
        cmd += contactDataOfUser(id );

    return cmd.toUtf8();
}

QByteArray Server::contactDataOfUser(int id )
{
    QSqlQuery query(*_database);
    QString clientStr = QString::number(id);
    QString command = "SELECT id , name , friends , isonline , lastseen , (blocked @> ARRAY[%1]) as isblocked , "
                      "(requests @> ARRAY[%2]) as hasSentRequest FROM appusers WHERE id = " + QString::number(id) + ";";
    command = command.arg(clientStr).arg(clientStr);
    query.prepare(command);
    query.exec();
    query.next();
    QString result;

    //if(clientId == query.value(0).toInt()) continue;

    result += '{';
    result += contact_idSep         + '"' + query.value(0).toString() + '"';
    result += contact_nameSep       + '"' + query.value(1).toString() + '"';
    result += contact_friendListSep + '"' + query.value(2).toString() + '"';
    result += contact_onlineSep     + '"' + query.value(3).toString() + '"';
    result += contact_lastSeenSep   + '"' + query.value(4).toString() + '"';
    result += contact_isBlockedSep  + '"' + query.value(5).toString() + '"';
    result += contact_hasRequestSep + '"' + query.value(6).toString() + '"';
    result += '}';

    return result.toUtf8();
}

void Server::sendNewChatToUsers(const QString& id1, const QString& id2, const QString& chatId)
{
    //here I would send also the contact data of each other , but right now , I pressume the fact that , if they can make a private chat , they are at least
    // AQUAITANCES
    std::shared_ptr<Client> client1 = _list.clientById(id1.toInt());
    std::shared_ptr<Client> client2 = _list.clientById(id2.toInt());

    QString cmd;
    QSqlQuery query(*_database);
    query.exec("SELECT * FROM rooms WHERE id = " + chatId + ';');
    query.next();
    if (client1.get() && client2.get())
    {
        cmd += chat_idSep + '"' + chatId + '"';
        cmd += chat_nameSep + '"' + query.value(1).toString() + '"';
        cmd += chat_historySep + '"' + formMessageHistory(chatId.toInt()) + '"';
        cmd += chat_memberListSep + '"' + query.value(3).toString() + '"';
        cmd += chat_privateSep + '"' + query.value(4).toString() + '"';
        //extended 
        cmd += chat_adminIdSep + Aux::quoteIt(query.value(5).toString());
        cmd += chat_readOnlyListSep + Aux::quoteIt(query.value(6).toString());
        //
        cmd += chat_isSenderSep + "\"%1\"";
        Aux::createCmd(cmd, InfoToClient::NewChat);

        client1->socket->write(cmd.arg("true").toUtf8());
        client1->socket->flush();
        client2->socket->write(cmd.arg("false").toUtf8());
        client2->socket->flush();
    }
    //if the first fails , it means only the requester is online
    else
    {
        cmd += chat_idSep + '"' + chatId + '"';
        cmd += chat_nameSep + Aux::quoteIt(query.value(1).toString());
        cmd += chat_historySep + formMessageHistory(chatId.toInt());
        cmd += chat_memberListSep + Aux::quoteIt(query.value(3).toString());
        cmd += chat_privateSep + Aux::quoteIt(query.value(4).toString());
        //extended 
        cmd += chat_adminIdSep + Aux::quoteIt(query.value(5).toString());
        cmd += chat_readOnlyListSep + Aux::quoteIt(query.value(6).toString());
        //
        cmd += chat_isSenderSep + "\"true\"";
        Aux::createCmd(cmd, InfoToClient::NewChat);
        client1->socket->write(cmd.toUtf8());
        client1->socket->flush();
    }

}


QString Server::createGroupChat(std::vector<QString> idList)
{
    QSqlQuery query(*_database);
    QString cmd = "INSERT INTO rooms (name , history , memberlist , private , admin) VALUES"
        "('Group chat' , ARRAY[('Server' , 'Come on , break the ice' , CURRENT_TIMESTAMP)]::message[] , ARRAY[%1] , 'false' , " + idList.front() + ") RETURNING id;";

    QString listStr = idList.front();
    for (int i = 1; i < idList.size(); i++)
        listStr += ','  + idList[i] ;
    cmd = cmd.arg(listStr);
 
    query.exec(std::move(cmd));
    query.next();
    return query.value(0).toString();
}

void Server::sendNewChatToUsers(std::vector<QString> list, const QString& chatId)
{
    QSqlQuery query(*_database);
    query.exec("SELECT * FROM rooms WHERE id = " + chatId + ';');
    query.next();
    //contact info of all the chat users
    std::vector<int> idList = Aux::extractIntArray(query.value(3).toString());
    QString necessaryContacts = contactDataOfUsers(idList);
    Aux::createCmd(necessaryContacts, InfoToClient::NecessaryContacts);
    _list.sendMessageToUsers(list, necessaryContacts.toUtf8());

    //the info of the chat itself
    std::shared_ptr<Client> admin = _list.clientById(list[0].toInt());
    QString cmd;
    cmd += chat_idSep + '"' + chatId + '"';
    cmd += chat_nameSep + '"' + query.value(1).toString() + '"';
    cmd += chat_historySep + '"' + formMessageHistory(chatId.toInt()) + '"';
    cmd += chat_memberListSep + '"' + query.value(3).toString() + '"';
    cmd += chat_privateSep + '"' + query.value(4).toString() + '"';
    //extended
    cmd += chat_adminIdSep + Aux::quoteIt(query.value(5).toString());
    cmd += chat_readOnlyListSep + Aux::quoteIt(query.value(6).toString());
    //
    cmd += chat_isSenderSep + "\"%1\"";
    Aux::createCmd(cmd, InfoToClient::NewChat);

    if (admin.get())
    {
        admin->socket->write(cmd.arg("true").toUtf8());
        admin->socket->flush();
    }
    list.erase(list.begin());

    _list.sendMessageToUsers(list, cmd.toUtf8());

}

void Server::sendServerAnnouncementToChat(QString chatId, const QString& text)
{
    QSqlQuery query(*_database);
    query.exec("SELECT memberlist FROM rooms WHERE id = " + chatId + ';');
    query.next();
    std::vector<int> list = Aux::extractIntArray(query.value(0).toString());

    QString cmd = "UPDATE rooms SET history = array_append(history , ROW('%1' , '%2' , '%3')::message) WHERE id = " + chatId + ';';
    QDateTime timestamp = QDateTime::currentDateTime();
    QString timestampStr = timestamp.date().toString("yyyy-MM-dd") + ' ' + timestamp.time().toString();
    QString name = "Server";
    cmd = cmd.arg(name).arg(text).arg(timestampStr);

    query.exec(cmd);
    cmd = formNewMessageInfo(chatId.toInt(), name, text, timestampStr);

    _list.sendMessageToUsers(list, cmd);
}

void Server::changeChatName(const QString& chatId, const QString& newName)
{
    QString cmd = "UPDATE rooms SET name = '" + newName + "' WHERE id = " + chatId  + " RETURNING memberlist;";
    QSqlQuery query(*_database);

    query.exec(cmd);
    query.next();

    cmd = chat_idSep + Aux::quoteIt(chatId) + chat_nameSep + Aux::quoteIt(newName);
    Aux::createCmd(cmd, InfoToClient::NewChatName);
    std::vector<int> list = Aux::extractIntArray(query.value(0).toString());
    _list.sendMessageToUsers(list, cmd);

    cmd = "UPDATE rooms SET history = array_append(history , ROW('%1' , '%2' , '%3')::message) WHERE id = " + chatId + ';';
    QDateTime timestamp = QDateTime::currentDateTime();
    QString timestampStr = timestamp.date().toString("yyyy-MM-dd") + ' ' + timestamp.time().toString();
    QString message = "Chat name changed to " + newName;
    QString name = "Server";
    cmd = cmd.arg(name).arg(message).arg(timestampStr);

    query.exec(cmd);
    cmd = formNewMessageInfo(chatId.toInt(), name, message, timestampStr);

    _list.sendMessageToUsers(list, cmd);
}

void Server::sendMessageToFriendsOfUser(int userId , const QString& str)
{
    QSqlQuery query(*_database);
    query.exec("SELECT friends FROM appusers WHERE id = " + QString::number(userId) + ';');
    query.next();

    std::vector<int> list = extractIntsFromArr(query.value(0).toString());
    _list.sendMessageToUsers(list, str);
}

void Server::setStatusForUser(int id, bool online)
{
    QString userIdStr = QString::number(id);
    QSqlQuery query(*_database);
    QString queryStr = "UPDATE appusers SET isonline = '%1' WHERE id = %2;" , statusStr;
    statusStr = (online) ? "true" : "false";

    query.exec(queryStr.arg(statusStr).arg(userIdStr));

    QString cmd = contact_idSep + Aux::quoteIt(userIdStr) + contact_onlineSep + Aux::quoteIt(statusStr);
    Aux::createCmd(cmd, InfoToClient::FriendStatus);

    sendMessageToFriendsOfUser(id , cmd);
}
