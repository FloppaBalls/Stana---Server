
#include <QTcpSocket>
#include <queue>
#include <QImage>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include "MediaData.h"

class MediaUploader {
public:
	MediaUploader() = default;
	MediaUploader(QTcpSocket* socket0);
	void addMediaToQueue(std::unique_ptr<MediaData> data);
	void nextMedia(QString id);
	void nextChunk();
	void requestId(const QString& suf);
	void requestId(int extension);
private:
	static constexpr qsizetype chunkSize = 1024; // bytes
	std::queue<std::unique_ptr<MediaData>> mediaQueue;
	int currentChunk;
	int nChunks;

	std::unique_ptr<MediaData> currentData;
	QString currentId;
	QTcpSocket* socket;

	int bytesUploaded = 0;
};