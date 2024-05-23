#include "MediaUploader.h"
#include "Auxiliary.h"

MediaUploader::MediaUploader(QTcpSocket* socket0) :  socket(socket0)
{
	currentChunk = nChunks = 0;
}
void MediaUploader::addMediaToQueue(std::unique_ptr<MediaData> data)
{
	bool wasEmpty = mediaQueue.size() == 0;

	if (wasEmpty)
		requestId(data->extension);

	mediaQueue.emplace(std::move(data));
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

		qDebug() << "~~Uploading chunk of size: " << data.length();
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
		qDebug() << "~~Uploading chunk of size: " << data.length(); 
			qDebug() << data;
		//qDebug() << "Uploaded " << currentData->blob.length() << " bytes";

		if (mediaQueue.size())
		{
			std::unique_ptr<MediaData>& data = mediaQueue.front();
			requestId(data->extension);
		}
	}

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
