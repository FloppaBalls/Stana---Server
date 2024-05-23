#pragma once
#include <QByteArray>

struct MediaData {
	MediaData(QByteArray blob, qsizetype id, int extension);
	MediaData(qsizetype id, int extension);
	QByteArray blob;
	qsizetype id;
	int extension;
};