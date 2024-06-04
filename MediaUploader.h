
#include <QTcpSocket>
#include <queue>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include "MediaData.h"

class MediaUploader {
public:
	MediaUploader() = default;
	MediaUploader(QTcpSocket* socket0);
	void addMediaToQueue(std::unique_ptr<MediaHandlingData> data);
	void nextMedia(QString id);
	void nextChunk();
	void requestId(const QString& suf);
	void requestId(int extension);
	//returns it and deletes it from the info list
	std::unique_ptr<MediaUploadingData> readyInfo(QByteArray id);
private:
	static constexpr qsizetype chunkSize = 1024; // bytes
	std::queue<std::unique_ptr<MediaUploadingData>> mediaQueue;
	int currentChunk;
	int nChunks;

	std::unique_ptr<MediaUploadingData> currentData;
	QString currentId;
	QTcpSocket* socket;

	int bytesUploaded = 0;

	std::vector<std::unique_ptr<MediaUploadingData>> infoList;
};