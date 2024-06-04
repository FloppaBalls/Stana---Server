#include "MediaHandler.h"
#include "Auxiliary.h"

extern const QByteArray chunk_acceptedSep;

MediaHandler::MediaHandler() : sequence(0) , _mediaReady(false)
{}
void MediaHandler::addChunk(QTcpSocket& socket , qsizetype id, QByteArray& chunk)
{
	qsizetype left = 0, right = mediaList.size() - 1 , mid;
	bool found = false;
	bool fin = (chunk.back() == '1') ? true : false;
	chunk.removeLast();

	qsizetype pos = 0;
	while(left <= right)
	{
		mid = (left + right) / 2;
		if (mediaList[mid].handlingId < id)
		{
			left = mid + 1;
		}
		else if (id < mediaList[mid].handlingId)
		{
			right = mid - 1;
		}
		else
		{
			found = true;
			pos = mid;
			break;
		}
	}
	if (found)
	{
		mediaList[pos].blob.append(std::move(chunk));
		if (fin)
		{
			qDebug() << "~~Received final chunk";
			_readyMedia = std::make_unique<MediaHandlingData>(std::move(mediaList[pos]));
			qDebug() << "~~" << _readyMedia->blob.size() << " bytes of media uploaded";
			mediaList.erase(mediaList.begin() + pos);
			_mediaReady = true;
		}
		else
			qDebug() << "~~Received chunk";

		socket.write(ack);
		socket.flush();
	}
	else
	{
		socket.write(rej);
		socket.flush();
	}
}

qsizetype MediaHandler::makeNewSlot(int extension ,int chatId, int senderId, std::vector<int> members)
{
	mediaList.emplace_back(MediaHandlingData(sequence, extension , chatId , senderId , members));
	sequence++;
	return sequence - 1;
}

std::unique_ptr<MediaHandlingData> MediaHandler::readyMedia()
{
	if (_mediaReady)
	{
		_mediaReady = false;
		return std::move(_readyMedia);
	}
	else
		return nullptr;
}
bool MediaHandler::mediaReady() const { return _mediaReady; }