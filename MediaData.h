#pragma once
#include <QByteArray>

struct MediaHandlingData {
	MediaHandlingData();
	MediaHandlingData(QByteArray blob, qsizetype hid, int extension);
	MediaHandlingData(qsizetype id, int extension , int chatId , int senderId , std::vector<int> memebers0);
	QByteArray blob;
	qsizetype handlingId;
	int extension;
	std::vector<int> members;
	int chatId;
	int senderId;
};
struct MediaUploadingData : public MediaHandlingData{
	MediaUploadingData();
	MediaUploadingData(QByteArray blob, qsizetype uid, qsizetype hid, int extension);
	MediaUploadingData(MediaHandlingData data , int uploadingData);
	MediaUploadingData(MediaHandlingData data);
	MediaUploadingData(qsizetype uid, int extension);

	qsizetype uploadId;
};

struct MediaDataHandlingInfo{
	MediaDataHandlingInfo();
	MediaDataHandlingInfo(qsizetype uid, qsizetype hid, bool ready , bool uploaded);
	qsizetype uploadId;
	qsizetype handlingId;
	bool ready;
	bool uploaded;
};