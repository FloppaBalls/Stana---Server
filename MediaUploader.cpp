#include "MediaUploader.h"
#include "Auxiliary.h"

MediaUploader::MediaUploader(QTcpSocket* socket0) :  socket(socket0)
{
	currentChunk = nChunks = 0;
}
void MediaUploader::addMediaToQueue(std::unique_ptr<MediaHandlingData> data)
{
	bool wasEmpty = mediaQueue.size() == 0;

	if (wasEmpty)
		requestId(data->extension);

	mediaQueue.emplace(std::make_unique<MediaUploadingData>(*std::move(data)));
}
void MediaUploader::nextMedia(QString id)
{
	if (!mediaQueue.empty())
	{
		currentId = id;

		currentData = std::move(mediaQueue.front());
		nChunks = currentData->blob.length() / chunkSize;
		currentChunk = 0;

		if (currentData->blob.length() % chunkSize)
			nChunks++;

		bytesUploaded = 0;
		mediaQueue.pop();
		nextChunk();
	}
}

void MediaUploader::nextChunk()
{
	if (currentChunk < nChunks - 1)
	{
		QByteArray data = currentData->blob.sliced(currentChunk * chunkSize , chunkSize);
		data.append('0');
		bytesUploaded += data.size();
		qDebug() << "Bytes uploaded until now: " << bytesUploaded;
		currentChunk++;
		QByteArray message = QByteArray::number((int)RequestToMediaProvider::AddChunk) + '(' + currentId.toUtf8() + ',' + data + ')' + "\n";
		socket->write(message);
		socket->flush();
	}
	else if (currentChunk == nChunks - 1)
	{
		QByteArray data = currentData->blob.mid(currentChunk * chunkSize, chunkSize);
		data.append('1');
		bytesUploaded += data.size();
		currentChunk++;
		QByteArray message = QByteArray::number((int)RequestToMediaProvider::AddChunk) + '(' + currentId.toUtf8() + ',' + data + ')' + "\n";
		socket->write(message);
		socket->flush();

		qDebug() << "Uploaded " << currentData->blob.length() << " bytes";
		infoList.emplace_back(std::move(currentData));

		if (mediaQueue.size())
		{
			std::unique_ptr<MediaUploadingData>& data = mediaQueue.front();
			requestId(data->extension);
		}
	}

}


//returns it and deletes it from the info list
std::unique_ptr<MediaUploadingData> MediaUploader::readyInfo(QByteArray idStr)
{
	qsizetype id = idStr.toULongLong();
	for (int i = 0; i < infoList.size(); i++)
	{
		std::unique_ptr<MediaUploadingData> info = std::move(infoList[i]);
		if (info->uploadId == id)
		{
			infoList.erase(infoList.begin() + i);
			qDebug() << "ChatId: " << info->chatId << " SenderId: " << info->senderId << " HandlingId: " << info->handlingId;
			return std::move(info);
		}
	}
	qDebug() << "!!!HANDLING INFO NOT FOUND";

}

void MediaUploader::requestId(const QString& suf)
{
	qDebug() << "~~Requesting next ID for media...";
	socket->write(QByteArray::number((int)RequestToMediaProvider::MediaUploadId) + '(' + QByteArray::number(Converters::suffixType(suf)) + ')' + "\n");
	socket->flush();
}
void MediaUploader::requestId(int extension)
{
	qDebug() << "~~Requesting next ID for media...";
	socket->write(QByteArray::number((int)RequestToMediaProvider::MediaUploadId) + '(' + QByteArray::number(extension) + ')' + "\n");
	socket->flush();
}
