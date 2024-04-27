
#ifndef ROOM_H
#define ROOM_H
#include <QTcpSocket>

class Room : public QObject{
public:
    Room(QObject* pObject);
    void kickUser(const QString& key);
    void banUser();
private:
    std::vector<QTcpSocket*> _memebers;
};

#endif // ROOM_H
