#include "MediaHandler.h"
#include "Auxiliary.h"

extern const QByteArray chunk_acceptedSep;


MediaData::MediaData(QByteArray blob0, qsizetype id0, int extension0) : blob(blob0) , id(id0) , extension(extension0)
{}
MediaData::MediaData(qsizetype id0, int extension0) : id(id0) , extension(extension0)
{}

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
		if (mediaList[mid].id < id)
		{
			left = mid + 1;
		}
		else if (id < mediaList[mid].id)
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
			_readyMedia = std::make_unique<MediaData>(std::move(mediaList[pos]));
			qDebug() << "~~" << _readyMedia->blob.size() << " bytes of media uploaded";
			mediaList.erase(mediaList.begin() + pos);
		}
		else
			qDebug() << "~~Received chunk";

		QByteArray message = chunk_idSep + Auxiliary::quoteIt("true");
		Auxiliary::createCmd(message, InfoToClient::ChunkAccepted);
		socket.write(std::move(message));
		socket.flush();
		_mediaReady = true;

	}
	else
	{
		QByteArray message = chunk_idSep + Auxiliary::quoteIt("false");
		Auxiliary::createCmd(message, InfoToClient::ChunkAccepted);
		socket.write(std::move(message));
		socket.flush();

	}
}
qsizetype MediaHandler::makeNewSlot(int extension)
{
	mediaList.emplace_back(MediaData(sequence, extension));
	sequence++;
	return sequence - 1;
}

std::unique_ptr<MediaData> MediaHandler::readyMedia()
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