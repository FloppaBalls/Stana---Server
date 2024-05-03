#include "Server.h"
#include "Auxiliary.h"

using Aux = Auxiliary;


Message::Message()
{
    param.resize(3);
}
QByteArray& Message::operator[](int i) { return param[i];}


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

    QByteArray _databaseName = "sheeplydatabase",
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
    //    //extractFromChat(query.value(0).toByteArray());
    //    extractFromChat(query.value(0).toByteArray());
    //}

    //qDebug() << getChatsForUser(1);

}


void Server::extractFromChat(const QByteArray& str)
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
std::pair<Message, int> Server::message(const QByteArray& str , int start) const
{
    /* example of a message: "(\"Andrei alexander\",\"Ha ba la baie\",\"2021-03-05 11:34:05\")"
     * */
    int pos = start;
    const QByteArray paramSeparator = "\\\"";

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


    RequestFromClient cmdType = extractRequestType(message);
    QSqlQuery query(*_database);

    std::vector<QByteArray> paramList;
    std::vector<QByteArray> mediaParamList;

    QByteArray aux;
    QByteArray byteArr;
    switch(cmdType)
    {
    case RequestFromClient::Register:
        paramList = extractParameters(message);
        //verify if there is somebody already with this username or email

        if(query.exec("SELECT * FROM appusers WHERE name = '" + paramList[0] + "';") == false)
            qDebug() << "Find existing name query failed: " + query.lastError().text();

        if(query.next())
        {
            aux = commandBegin + "F" + QByteArray::number(int(RequestFailure::NameUsed));
            client->write(aux);
            break;
        }
        if(query.exec("SELECT * FROM appusers WHERE email = '" + paramList[1] + "';") == false)
            qDebug() << "Find existing email query failed: " + query.lastError().text();
        if(query.next())
        {
            aux = commandBegin +"F" + QByteArray::number(int(RequestFailure::EmailUsed));
            client->write(aux);
            break;
        }

        aux = "INSERT INTO appusers (name , email , password , isonline) VALUES ('" + paramList.at(0) + "' , '" + paramList.at(1) + "' , '" + paramList.at(2) + "','true') RETURNING id;";
        qDebug() << aux;
        query.exec(aux);
        _list.setIdForClient(Aux::getClientKey(client), aux.toInt());

        //aux = "I" + QByteArray::number(int(InfoToClient::SignedIn));
        //client->write(aux);

        client->write(formAllDataOfUser(query.value(0).toInt()));
        client->flush();
        break;
    case RequestFromClient::Login:
        paramList = extractParameters(message);
        // check if it is a valid username

        if(query.exec("SELECT * FROM appusers WHERE name = '" + paramList[0] + "';") == false)
            qDebug() << "Find existing name query failed: " + query.lastError().text();

        if(query.next())
        {
            //password is the forth column in appusers table

            aux = query.value(3).toByteArray();
            //the third parameter should be the password
            if(aux == paramList[1])
            {
                aux = formAllDataOfUser(query.value(0).toInt());
                _list.setIdForClient(Aux::getClientKey(client) , query.value(0).toInt());
            }
            else
                aux = commandBegin + "F" + QByteArray::number(int(RequestFailure::IncorrectPassword)) + ';';
        }
        else
            aux = "~F" + QByteArray::number(int(RequestFailure::NameNotFound)) + ';';


        setStatusForUser(query.value(0).toInt(), true);
        client->write(aux);
        client->flush();
        break;
    case RequestFromClient::AddMessage:
        paramList = extractComplexParameters(message, cmdType);
        qDebug() << "Sending message...";

        //ex UPDATE rooms SET history = array_append(history , ROW('Daquaivius' , 'nah like fr fr fr' , '2033-04-05 00:00:00')::message) WHERE id = 10;
        aux = "UPDATE rooms SET history = array_append(history , ROW('" + paramList.at(1) + "' , '" + paramList.at(2) + "' , '" + paramList.at(3) + "')::message) WHERE id = " + paramList[0] + ';';
        query.prepare(aux);


        if(query.exec() == false)
            qDebug() << query.lastError();

        byteArr = formNewMessageInfo(paramList.at(0).toInt() , paramList.at(1) , paramList.at(2) , paramList.at(3));
        _list.sendMessageToUsers(clientIdsForRoom(paramList.at(0).toInt()) , byteArr );
        break;
    case RequestFromClient::SearchForUsers:
        paramList = extractParameters(message);
        aux = commandBegin + "I" + QByteArray::number(int(InfoToClient::SearchedUserList)) + ':' + searchForUsers(paramList[1] , paramList[0].toInt()) + commandEnd;
        client->write(aux);
        client->flush();
        break;
    case RequestFromClient::ManageFriendRequest:
        paramList = extractParameters(message);
        // commandNumber(userId , friendRequestUserId  , accepted)
        query.exec("UPDATE appusers SET requests = ARRAY_REMOVE(requests , " + paramList[1] + ") WHERE id = " + paramList[0] + ';');
        if(paramList[2].toInt())
        {
            makeFriends(paramList[0] , paramList[1]);
        }
        break;
    case RequestFromClient::SendFriendRequest:
        paramList = extractParameters(message);
        query.exec("UPDATE appusers SET requests = ARRAY_APPEND(requests , " + paramList[0] + ") WHERE id = " + paramList[1] + ';');
        aux = commandBegin + "I" + QByteArray::number(int(InfoToClient::NewFriendRequest)) + ':' + contactDataOfUser(paramList[0].toInt()) + commandEnd;
        _list.sendMessageToUser(paramList[1].toInt() , aux);
        break;
    case RequestFromClient::CreatePrivateChat:
        paramList = extractParameters(message);
        //example: 
        //INSERT INTO rooms (name , history , memberlist , private) VALUES ('Private' , ARRAY[('Server' , 'Come on , break the ice' , CURRENT_TIMESTAMP)]::message[] , 
        //ARRAY[4 , 5] , 'true') RETURNING id;
        aux = "INSERT INTO rooms (name , history , memberlist , private) VALUES" 
            "('Private' , ARRAY[('Server' , 'Come on , break the ice' , CURRENT_TIMESTAMP)]::message[] , ARRAY[" + paramList.at(0) + " , " + paramList.at(1) + "] , 'true') RETURNING id;";
        query.exec(aux);
        query.next();
        sendNewChatToUsers(paramList[0], paramList[1] , query.value(0).toByteArray());
        break;
    case RequestFromClient::RemoveFriend:
        paramList = extractParameters(message);
        breakFriendship(paramList[0], paramList[1]);
        break;
    case RequestFromClient::BlockPerson:
        paramList = extractParameters(message);
        blockUserForUser(paramList[0], paramList[1]);
        break;
    case RequestFromClient::GetInfoOfUsers:
        paramList = extractParameters(message);
        aux = paramList.back();
        paramList.pop_back();
        client->write(getContactInfoOfUsers(aux, paramList));
        client->flush();
        break;
    case RequestFromClient::UnblockUser:
        paramList = extractParameters(message);
        unblockUserForUser(paramList[0], paramList[1]);
        break;
    case RequestFromClient::CreateGroupChat:
        paramList = extractParameters(message);
        aux = createGroupChat(paramList);
        sendNewChatToUsers(paramList, aux);
        break;
    case RequestFromClient::UpdateChatName:
        paramList = extractComplexParameters(message, cmdType);
        changeChatName(paramList[0], paramList[1]);
        break;
    case RequestFromClient::RemoveFromGroup:
        paramList = extractParameters(message);
        removeFromGroup(paramList[0], paramList[1] , paramList[2].front() == 't');
        break;
    case RequestFromClient::AddPeopleToTheChat:
        paramList = extractParameters(message);
        //the chat id
        aux = paramList.back();
        paramList.pop_back();
        addPeopleToGroup(aux, paramList);
        break;
    case RequestFromClient::AddMedia:
        break;
    default:
        break;
    }
}

std::vector<QByteArray> Server::extractMediaParameters(const QByteArray& arr)
{
    return { "caca" };
}

void Server::addPeopleToGroup(QByteArray chatId, std::vector<QByteArray> idList)
{
    QByteArray cmd1 = chat_idSep + Aux::quoteIt(chatId) + chat_newMembersSep + Aux::quoteIt(Aux::makeList(idList, '{', '}'));
    Aux::createCmd(cmd1, InfoToClient::GroupMembersAdded);

    sendMessageToChatMembers(chatId, cmd1);

    QSqlQuery query(*_database);
    cmd1 = "UPDATE rooms SET memberlist = memberlist || ARRAY" + Aux::makeList(idList, '[', ']') + " WHERE id = " + chatId + ';';
    query.exec(cmd1);


    sendNewChatToUsers(idList, chatId);

    for (QByteArray strId : idList)
    {
        query.exec("SELECT name FROM appusers WHERE id = " + strId + ';');
        query.next();

        sendServerAnnouncementToChat(chatId, query.value(0).toByteArray() + " was added to the group");
    }
}

void Server::sendMessageToChatMembers(QByteArray chatId, const QByteArray& text)
{
    QSqlQuery query(*_database);
    query.exec("SELECT memberlist FROM rooms WHERE id = " + chatId + ';');
    query.next();

    std::vector<int> list = Aux::extractIntArray(query.value(0).toByteArray());
    _list.sendMessageToUsers(list, text);
}

void Server::removeFromGroup(QByteArray chatId, QByteArray userId, bool removed)
{
    QSqlQuery query(*_database);
    query.exec("SELECT memberlist FROM rooms WHERE id = " + chatId + ';');
    query.next();

    std::vector<int> list = Aux::extractIntArray(query.value(0).toByteArray());
    //removing the id from the memberlist
    query.exec("UPDATE rooms SET memberlist = array_remove( memberlist , " + userId + " ) WHERE id = " + chatId + " RETURNING admin;");
    query.next();
    // retaining the admin id for later
    int adminId = query.value(0).toInt();
    //sending to the group members the fact that the member was removed 
    QByteArray cmd = chat_idSep + Aux::quoteIt(chatId) + contact_idSep + Aux::quoteIt(userId) + chat_removedSep +
            Aux::quoteIt((removed) ? "true" : "false");

    Aux::createCmd(cmd, InfoToClient::GroupMemberRemoved);
    query.exec("SELECT name FROM appusers WHERE id = " + userId + ';');
    query.next();
    sendServerAnnouncementToChat(chatId, query.value(0).toByteArray() +
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
        cmd = chat_idSep + Aux::quoteIt(chatId) + contact_idSep + Aux::quoteIt(query.value(0).toByteArray());
        Aux::createCmd(cmd, InfoToClient::NewAdmin);

        _list.sendMessageToUsers(list, cmd);

        sendServerAnnouncementToChat(chatId, query.value(1).toByteArray() + " is the new Admin");
    }
}

void Server::unblockUserForUser(QByteArray id1, QByteArray id2)
{
    QSqlQuery query(*_database);
    query.exec("UPDATE appusers SET blocked = ARRAY_REMOVE(blocked ," + id2 + ") WHERE id = " + id1 + ';');

    auto client1 = _list.clientById(id1.toInt());
    if (client1.get())
    {
        QByteArray str = contact_idSep + Aux::quoteIt(id2);
        Aux::createCmd(str, InfoToClient::UserUnblocked);
        client1->socket->write(str);
        client1->socket->flush();
    }

    auto client2 = _list.clientById(id2.toInt());
    if (client2.get())
    {
        QByteArray str = contact_idSep + Aux::quoteIt(id1);
        Aux::createCmd(str, InfoToClient::UserUnblockedYou);
        client2->socket->write(str);
        client2->socket->flush();
    }
}

QByteArray Server::getContactInfoOfUsers(const QByteArray& id , std::vector<QByteArray> idList)
{
    QByteArray cmd = "SELECT id , name , friends , isonline , lastseen , (blocked @> ARRAY[" + id + "]) ," 
        "(requests @> ARRAY[" + id + "]) as hasSentRequest  FROM appusers WHERE id IN " + Auxiliary::makeList(idList) + ';';

    QSqlQuery query(*_database);
    if (query.exec(cmd) == false)
        qDebug() << "Nigga";

    //"SELECT id , name , friends , isonline , lastseen , (blocked @> ARRAY[%1]) as isblocked , "
    //"(requests @> ARRAY[%2]) as hasSentRequest FROM appusers WHERE name ILIKE '%" + str + "%';";
    QByteArray result = "{";
    while (query.next())
    {
        result += contact_idSep + '"' + query.value(0).toByteArray() + '"';
        result += contact_nameSep + '"' + query.value(1).toByteArray() + '"';
        result += contact_friendListSep + '"' + query.value(2).toByteArray() + '"';
        result += contact_onlineSep + '"' + query.value(3).toByteArray() + '"';
        result += contact_lastSeenSep + '"' + query.value(4).toByteArray() + '"';
        result += contact_isBlockedSep + '"' + query.value(5).toByteArray() + '"';
        result += contact_hasRequestSep + '"' + query.value(6).toByteArray() + '"';
    }
    result += '}';
    Auxiliary::createCmd(result, InfoToClient::InfoOfUsers);
    return result;
}

void Server::blockUserForUser(QByteArray id1, QByteArray id2)
{
    QSqlQuery query(*_database);
    query.exec("UPDATE appusers SET friends = ARRAY_REMOVE(friends ," + id1 + ") WHERE id = " + id2 + ';');
    query.exec("UPDATE appusers SET friends = ARRAY_REMOVE(friends ," + id2 + ") WHERE id = " + id1 + ';');
    query.exec("UPDATE appusers SET blocked = ARRAY_APPEND(blocked ," + id2 + ") WHERE id = " + id1 + ';');
    query.exec("UPDATE appusers SET requests = ARRAY_REMOVE(requests ," + id1 + ") WHERE id = " + id2 + ';');

    auto client1 = _list.clientById(id1.toInt());
    if (client1.get())
    {
        QByteArray str = contact_idSep + Aux::quoteIt(id2);
        Aux::createCmd(str, InfoToClient::PersonBlocked);
        client1->socket->write(str);
        client1->socket->flush();
    }

    auto client2 = _list.clientById(id2.toInt());
    if (client2.get())
    {
        QByteArray str = contact_idSep + Aux::quoteIt(id1);
        Aux::createCmd(str, InfoToClient::YouGotBlocked);
        client2->socket->write(str);
        client2->socket->flush();
    }
}
void Server::breakFriendship(QByteArray id1, QByteArray id2)
{
    QSqlQuery query(*_database);
    query.exec("UPDATE appusers SET friends = ARRAY_REMOVE(friends ," + id1 + ") WHERE id = " + id2 + ';');
    query.exec("UPDATE appusers SET friends = ARRAY_REMOVE(friends ," + id2 + ") WHERE id = " + id1 + ';');

    auto client1 = _list.clientById(id1.toInt());
    if (client1.get())
    {
        QByteArray str = contact_idSep + Aux::quoteIt(id2);
        Aux::createCmd(str, InfoToClient::FriendRemoved);
        client1->socket->write(str);
        client1->socket->flush();
    }

    auto client2 = _list.clientById(id2.toInt());
    if (client2.get())
    {
        QByteArray str = contact_idSep + Aux::quoteIt(id2);
        Aux::createCmd(str, InfoToClient::FriendRemoved);
        client2->socket->write(str);
        client2->socket->flush();
    }
}

void Server::makeFriends(int id1 , int id2)
{
    makeFriends(QByteArray::number(id1) ,QByteArray::number(id2));
}
void Server::makeFriends(QByteArray id1 , QByteArray id2)
{
    QSqlQuery query(*_database);
    query.exec("UPDATE appusers SET friends = ARRAY_APPEND(friends ," + id1 + ") WHERE id = " + id2 + ';');
    query.exec("UPDATE appusers SET friends = ARRAY_APPEND(friends ," + id2 + ") WHERE id = " + id1 + ';');


    /*      result += nameSep + '"' + userQuery.value(0).toByteArray() +'"';
            result += onlineSep + '"' + userQuery.value(1).toByteArray() +'"';
            result += lastSeenSep + '"' + userQuery.value(2).toByteArray() +'"';\
            result += friendListSep + '"' + userQuery.value(3).toByteArray() +'"';*/
    auto client1 = _list.clientById(id1.toInt());
    QByteArray text;
    if(client1.get())
    {
        query.exec("SELECT name , isonline , lastseen , friends from appusers WHERE id = " + id2 + ';');
        query.next();
        text = commandBegin + 'I' + QByteArray::number(int(InfoToClient::NewFriend)) +':';
        text += contact_idSep + '"' + id2 +'"' +
               contact_nameSep + '"' + query.value(0).toByteArray() +'"' +
               contact_onlineSep + '"' + query.value(1).toByteArray() +'"' +
               contact_lastSeenSep + '"' + query.value(2).toByteArray() +'"' +
               contact_friendListSep + '"' + query.value(3).toByteArray() +'"';
        text += commandEnd;
        client1->socket->write(text);
        client1->socket->flush();
    }

    auto client2 = _list.clientById(id2.toInt());
    if(client2.get())
    {
        query.exec("SELECT name , isonline  , lastseen , friends from appusers WHERE id = " + id1 + ';');
        query.next();

        text = commandBegin + 'I' + QByteArray::number(int(InfoToClient::NewFriend)) +':';
        text += contact_idSep + '"' + id1 +'"' +
               contact_nameSep + '"' + query.value(0).toByteArray() +'"' +
               contact_onlineSep + '"' + query.value(1).toByteArray() +'"' +
               contact_lastSeenSep + '"' + query.value(2).toByteArray() +'"' +
               contact_friendListSep + '"' + query.value(3).toByteArray() +'"';
        text += commandEnd;
        client2->socket->write(text);
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
    QByteArray info = QByteArray( commandBegin + "I0:" + getSignInInfoForUser(userId) + commandEnd) +
                   commandBegin + "I1:" + getChatsForUser(userId) + commandEnd +
                   commandBegin + "I2:" + getContactsAndTheirInfoForUser(userId) + commandEnd;
    return info;
}
QByteArray Server::getSignInInfoForUser(int id)
{
    QByteArray str = user_idSep + '"' + QByteArray::number(id) + '"';
    return str;
}

QByteArray Server::getChatsForUser(int id)
{
    QSqlQuery query(*_database);
    query.prepare("SELECT id , name , private , memberlist , admin , read_only_members FROM rooms WHERE :id = ANY(memberlist);");
    query.bindValue(":id" , id);
    query.exec();

    QByteArray chats;

    while(query.next())
    {
        chats += chat_idSep           + Aux::quoteIt(query.value(0).toByteArray());
        chats += chat_nameSep         + Aux::quoteIt(query.value(1).toByteArray());
        chats += chat_historySep      + Aux::quoteIt(formMessageHistory(query.value(0).toInt()));
        chats += chat_privateSep      + Aux::quoteIt(query.value(2).toByteArray());
        chats += chat_memberListSep   + Aux::quoteIt(query.value(3).toByteArray());
        //extended
        chats += chat_adminIdSep      + Aux::quoteIt(query.value(4).toByteArray());
        chats += chat_readOnlyListSep + Aux::quoteIt(query.value(5).toByteArray());
    }
    return chats;
}

QByteArray Server::formMessageHistory(int chatId)
{
    QSqlQuery query(*_database);
    query.prepare("SELECT (unnest(r.history)).* FROM rooms r WHERE id = :id;");
    query.bindValue(":id" , chatId);
    query.exec();

    QByteArray str = "{";
    while(query.next())
    {
        str += '(';
        str += message_nameSep      + query.value(0).toByteArray();
        str += message_textSep      + query.value(1).toByteArray();
        //for some reason , when returning converting a TIMESTAMP value to ByteArray , it returns null. So I instead convert it to string and then byteArray
        str += message_timestampSep + query.value(2).toString().replace('T' , ' ').toUtf8();
        str += ')';
    }
    str += '}';
    return str;
}

QByteArray Server::formNewMessageInfo(int chatId , QByteArray sender , QByteArray text , QByteArray timestamp)
{
    QByteArray str (commandBegin + 'I' + QByteArray::number((int)InfoToClient::NewMessage) + ':' +
                message_chatIdSep + "\"" + QByteArray::number(chatId) + "\"" + message_nameSep + "\"" + sender + "\"" + 
                message_textSep + Aux::quoteIt(text.replace('"' , "\\\"")) +
                message_timestampSep + "\"" + timestamp + "\"" + commandEnd);

    return str;
}

QByteArray Server::getContactsAndTheirInfoForUser( int id)
{
    QSqlQuery query(*_database);
    query.exec("SELECT friends , requests , blocked FROM appusers WHERE id = " + QByteArray::number(id));
    query.next();

    QByteArray result;

    auto replace = [](QByteArray& str){
        str = str.replace('{' , '(').replace('}' , ')');
    };
    std::vector<QByteArray> lists(3);
    for(auto i = 0 ; i < lists.size() ;i++)
    {
        result += contactSeps[i];
        lists[i] = query.value(i).toByteArray();
        replace(lists[i]);
        QSqlQuery userQuery;
        userQuery.exec("SELECT id , name , isonline , lastseen , friends , requests from appusers WHERE id IN " + lists[i] + ';');

        while(userQuery.next())
        {
            result += contact_idSep + '"' + userQuery.value(0).toByteArray() +'"';
            result += contact_nameSep + '"' + userQuery.value(1).toByteArray() +'"';
            result += contact_onlineSep + '"' + userQuery.value(2).toByteArray() +'"';
            result += contact_lastSeenSep + '"' + userQuery.value(3).toByteArray() +'"';\
            result += contact_friendListSep + '"' + userQuery.value(4).toByteArray() +'"';
        }
    }
    qDebug() << result;
    return result;
}
std::vector<QByteArray> Server::extractParameters(const QByteArray& str)
{
    int paramBeg = str.indexOf('(');
    int paramEnd = str.indexOf(')');

    if(paramEnd == -1 || paramBeg == -1)
        qDebug() << "Error: Did not provide parantheses for parameters";

    std::vector<QByteArray> parameterList;
    QByteArray aux;
    for(int i = paramBeg + 1 ;  i < paramEnd ; i++)
    {
        char ch = str[i];
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

std::vector<QByteArray> Server::extractComplexParameters(const QByteArray& str, RequestFromClient type)
{
    const QByteArray paramMessageBegStr = " (";
    const QByteArray paramMessageEndStr = " )";
    const QByteArray paramSep = " ,";
    int paramBeg = str.indexOf(paramMessageBegStr);
    int paramEnd = str.indexOf(paramMessageEndStr);

    if (paramEnd == -1 || paramBeg == -1)
        qDebug() << "Error: Did not provide parantheses for parameters";

    std::vector<QByteArray> parameterList;
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
RequestFromClient Server::extractRequestType(const QByteArray& str)
{
    //begginging of paramaters
    int paramBeg = str.indexOf('(') ;
    //string form of the command number
    QByteArray cmdStr;
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
    query.exec("SELECT memberlist FROM rooms WHERE id = " + QByteArray::number(roomId) + ';');
    query.next();
    QByteArray str = query.value(0).toByteArray();

    std::vector<int> memberList(extractIntsFromArr(str));
    return memberList;
}

std::vector<int> Server::extractIntsFromArr(const QByteArray& str)
{
    std::vector<int> memberList;
    QByteArray nrStr;
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

QByteArray Server::searchForUsers(const QByteArray& str , int clientId)
{
    QSqlQuery query(*_database);
    QByteArray clientStr = QByteArray::number(clientId);
    QByteArray command = "SELECT id , name , friends , isonline , lastseen , (blocked @> ARRAY[" + clientStr + "]) as isblocked , "
                      "(requests @> ARRAY[" + clientStr + "]) as hasSentRequest FROM appusers WHERE name ILIKE '%" + str + "%';";

    query.prepare(command);

    query.exec();
    QByteArray result;

    while(query.next())
    {
        //if(clientId == query.value(0).toInt()) continue;

        result += '{';
        result += contact_idSep         + '"' + query.value(0).toByteArray() + '"';
        result += contact_nameSep       + '"' + query.value(1).toByteArray() + '"';
        result += contact_friendListSep + '"' + query.value(2).toByteArray() + '"';
        result += contact_onlineSep     + '"' + query.value(3).toByteArray() + '"';
        result += contact_lastSeenSep   + '"' + query.value(4).toByteArray() + '"';
        result += contact_isBlockedSep  + '"' + query.value(5).toByteArray() + '"';
        result += contact_hasRequestSep + '"' + query.value(6).toByteArray() + '"';
        result += '}';
    }

    return result;
}

QByteArray Server::contactDataOfUsers(std::vector<int> idList )
{
    QByteArray cmd;
    for (int id : idList)
        cmd += contactDataOfUser(id );

    return cmd;
}

QByteArray Server::contactDataOfUser(int id )
{
    QSqlQuery query(*_database);
    QByteArray clientStr = QByteArray::number(id);
    QByteArray command = "SELECT id , name , friends , isonline , lastseen , (blocked @> ARRAY[" + clientStr + "]) as isblocked , "
                      "(requests @> ARRAY[" + clientStr + "]) as hasSentRequest FROM appusers WHERE id = " + QByteArray::number(id) + ";";

    query.prepare(command);
    query.exec();
    query.next();
    QByteArray result;

    //if(clientId == query.value(0).toInt()) continue;

    result += '{';
    result += contact_idSep         + '"' + query.value(0).toByteArray() + '"';
    result += contact_nameSep       + '"' + query.value(1).toByteArray() + '"';
    result += contact_friendListSep + '"' + query.value(2).toByteArray() + '"';
    result += contact_onlineSep     + '"' + query.value(3).toByteArray() + '"';
    result += contact_lastSeenSep   + '"' + query.value(4).toByteArray() + '"';
    result += contact_isBlockedSep  + '"' + query.value(5).toByteArray() + '"';
    result += contact_hasRequestSep + '"' + query.value(6).toByteArray() + '"';
    result += '}';

    return result;
}

void Server::sendNewChatToUsers(const QByteArray& id1, const QByteArray& id2, const QByteArray& chatId)
{
    //here I would send also the contact data of each other , but right now , I pressume the fact that , if they can make a private chat , they are at least
    // AQUAITANCES
    std::shared_ptr<Client> client1 = _list.clientById(id1.toInt());
    std::shared_ptr<Client> client2 = _list.clientById(id2.toInt());

    QByteArray cmd1 , cmd2;
    QSqlQuery query(*_database);
    query.exec("SELECT * FROM rooms WHERE id = " + chatId + ';');
    query.next();
    if (client1.get() && client2.get())
    {
        cmd1 += chat_idSep + '"' + chatId + '"';
        cmd1 += chat_nameSep + '"' + query.value(1).toByteArray() + '"';
        cmd1 += chat_historySep + '"' + formMessageHistory(chatId.toInt()) + '"';
        cmd1 += chat_memberListSep + '"' + query.value(3).toByteArray() + '"';
        cmd1 += chat_privateSep + '"' + query.value(4).toByteArray() + '"';
        //extended 
        cmd1 += chat_adminIdSep + Aux::quoteIt(query.value(5).toByteArray());
        cmd1 += chat_readOnlyListSep + Aux::quoteIt(query.value(6).toByteArray());
        //
        cmd2 = cmd1 + chat_isSenderSep + "\"false\"";
        cmd1 += chat_isSenderSep + "\"true\"";

        Aux::createCmd(cmd1, InfoToClient::NewChat);
        Aux::createCmd(cmd2, InfoToClient::NewChat);

        client1->socket->write(cmd1);
        client1->socket->flush();
        client2->socket->write(cmd2);
        client2->socket->flush();
    }
    //if the first fails , it means only the requester is online
    else
    {
        cmd1 += chat_idSep + '"' + chatId + '"';
        cmd1 += chat_nameSep + Aux::quoteIt(query.value(1).toByteArray());
        cmd1 += chat_historySep + formMessageHistory(chatId.toInt());
        cmd1 += chat_memberListSep + Aux::quoteIt(query.value(3).toByteArray());
        cmd1 += chat_privateSep + Aux::quoteIt(query.value(4).toByteArray());
        //extended 
        cmd1 += chat_adminIdSep + Aux::quoteIt(query.value(5).toByteArray());
        cmd1 += chat_readOnlyListSep + Aux::quoteIt(query.value(6).toByteArray());
        //
        cmd1 += chat_isSenderSep + "\"true\"";
        Aux::createCmd(cmd1, InfoToClient::NewChat);
        client1->socket->write(cmd1);
        client1->socket->flush();
    }

}


QByteArray Server::createGroupChat(std::vector<QByteArray> idList)
{
    QSqlQuery query(*_database);

    QByteArray listStr = idList.front();
    for (int i = 1; i < idList.size(); i++)
        listStr += ','  + idList[i] ;
    QByteArray cmd = "INSERT INTO rooms (name , history , memberlist , private , admin) VALUES"
        "('Group chat' , ARRAY[('Server' , 'Come on , break the ice' , CURRENT_TIMESTAMP)]::message[] , ARRAY[" + listStr + "] , 'false' , " + idList.front() + ") RETURNING id;";

    query.exec(std::move(cmd));
    query.next();
    return query.value(0).toByteArray();
}

void Server::sendNewChatToUsers(std::vector<QByteArray> list, const QByteArray& chatId)
{
    QSqlQuery query(*_database);
    query.exec("SELECT * FROM rooms WHERE id = " + chatId + ';');
    query.next();
    //contact info of all the chat users
    std::vector<int> idList = Aux::extractIntArray(query.value(3).toByteArray());
    QByteArray necessaryContacts = contactDataOfUsers(idList);
    Aux::createCmd(necessaryContacts, InfoToClient::NecessaryContacts);
    _list.sendMessageToUsers(list, necessaryContacts);

    //the info of the chat itself
    std::shared_ptr<Client> admin = _list.clientById(list[0].toInt());
    QByteArray cmd1 , cmd2;
    cmd1 += chat_idSep + '"' + chatId + '"';
    cmd1 += chat_nameSep + '"' + query.value(1).toByteArray() + '"';
    cmd1 += chat_historySep + '"' + formMessageHistory(chatId.toInt()) + '"';
    cmd1 += chat_memberListSep + '"' + query.value(3).toByteArray() + '"';
    cmd1 += chat_privateSep + '"' + query.value(4).toByteArray() + '"';
    //extended
    cmd1 += chat_adminIdSep + Aux::quoteIt(query.value(5).toByteArray());
    cmd1 += chat_readOnlyListSep + Aux::quoteIt(query.value(6).toByteArray());
    cmd2 = cmd1;
    //
    cmd1 += chat_isSenderSep + "\"true\"";
    cmd2 += chat_isSenderSep + "\"false\"";
    Aux::createCmd(cmd1, InfoToClient::NewChat);
    Aux::createCmd(cmd2, InfoToClient::NewChat);

    if (admin.get())
    {
        admin->socket->write(cmd1);
        admin->socket->flush();
    }
    list.erase(list.begin());

    _list.sendMessageToUsers(list, cmd2);

}

void Server::sendServerAnnouncementToChat(QByteArray chatId, const QByteArray& text)
{
    QSqlQuery query(*_database);
    query.exec("SELECT memberlist FROM rooms WHERE id = " + chatId + ';');
    query.next();
    std::vector<int> list = Aux::extractIntArray(query.value(0).toByteArray());

    QDateTime timestamp = QDateTime::currentDateTime();
    QByteArray timestampStr = (timestamp.date().toString("yyyy-MM-dd") + ' ' + timestamp.time().toString()).toUtf8();
    QByteArray name = "Server";
    QByteArray cmd = "UPDATE rooms SET history = array_append(history , ROW('" + name + "' , '" + text +  "' , '" + timestampStr + "')::message) WHERE id = " + chatId + ';';


    query.exec(cmd);
    cmd = formNewMessageInfo(chatId.toInt(), name, text, timestampStr);

    _list.sendMessageToUsers(list, cmd);
}

void Server::changeChatName(const QByteArray& chatId, const QByteArray& newName)
{
    QByteArray cmd = "UPDATE rooms SET name = '" + newName + "' WHERE id = " + chatId  + " RETURNING memberlist;";
    QSqlQuery query(*_database);

    query.exec(cmd);
    query.next();

    cmd = chat_idSep + Aux::quoteIt(chatId) + chat_nameSep + Aux::quoteIt(newName);
    Aux::createCmd(cmd, InfoToClient::NewChatName);
    std::vector<int> list = Aux::extractIntArray(query.value(0).toByteArray());
    _list.sendMessageToUsers(list, cmd);

    QDateTime timestamp = QDateTime::currentDateTime();
    QByteArray timestampStr = (timestamp.date().toString("yyyy-MM-dd") + ' ' + timestamp.time().toString()).toUtf8();
    QByteArray message = "Chat name changed to " + newName;
    QByteArray name = "Server";
    cmd = "UPDATE rooms SET history = array_append(history , ROW('" + name + "' , '" + message + "' , '"  + timestampStr + "')::message) WHERE id = " + chatId + ';';

    query.exec(cmd);
    cmd = formNewMessageInfo(chatId.toInt(), name, message, timestampStr);

    _list.sendMessageToUsers(list, cmd);
}

void Server::sendMessageToFriendsOfUser(int userId , const QByteArray& str)
{
    QSqlQuery query(*_database);
    query.exec("SELECT friends FROM appusers WHERE id = " + QByteArray::number(userId) + ';');
    query.next();

    std::vector<int> list = extractIntsFromArr(query.value(0).toByteArray());
    _list.sendMessageToUsers(list, str);
}

void Server::setStatusForUser(int id, bool online)
{
    QByteArray userIdStr = QByteArray::number(id);
    QSqlQuery query(*_database);
    QString queryStr = "UPDATE appusers SET isonline = '%1' WHERE id = %2;" , statusStr;
    statusStr = (online) ? "true" : "false";

    query.exec(queryStr.arg(statusStr).arg(userIdStr));

    QByteArray cmd = contact_idSep + Aux::quoteIt(userIdStr) + contact_onlineSep + Aux::quoteIt(statusStr.toUtf8());
    Aux::createCmd(cmd, InfoToClient::FriendStatus);

    sendMessageToFriendsOfUser(id , cmd);
}
