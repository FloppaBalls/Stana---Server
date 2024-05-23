#include "MediaData.h"

MediaData::MediaData(QByteArray blob0, qsizetype id0, int extension0) : blob(blob0), id(id0), extension(extension0)
{}
MediaData::MediaData(qsizetype id0, int extension0) : id(id0), extension(extension0)
{}
