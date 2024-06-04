#include <QByteArray>
#include <qDebug>
#include <QTcpSocket>
#include "MediaData.h"

class MediaHandler {
public:
	MediaHandler();
	bool mediaReady() const;
	//the last byte of the chunk should represent if it is the final chunk or not
	void addChunk(QTcpSocket& socket , qsizetype id , QByteArray& chunk );
	qsizetype makeNewSlot( int extension , int chatId , int senderId , std::vector<int> members);
	std::unique_ptr<MediaHandlingData> readyMedia();
private:
	qsizetype sequence;
	std::vector<MediaHandlingData> mediaList;
	
	std::unique_ptr<MediaHandlingData> _readyMedia;
	bool _mediaReady;
};