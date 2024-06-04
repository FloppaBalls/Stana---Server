#include "MediaData.h"


MediaHandlingData::MediaHandlingData()
	:handlingId(0) , extension(-1) 
{}
MediaHandlingData::MediaHandlingData(QByteArray blob0, qsizetype hid, int extension0)
	:handlingId(hid) , blob(blob0) , extension(extension0)
{}
MediaHandlingData::MediaHandlingData(qsizetype id, int extension0 , int chatId0, int senderId0, std::vector<int> members0)
	:handlingId(id) , extension(extension0) , chatId(chatId0) , senderId(senderId0) , members(members0)
{}


MediaUploadingData::MediaUploadingData()
	: MediaHandlingData(0 , -1 , 0 , 0 , {}), uploadId(0)
{}
MediaUploadingData::MediaUploadingData(QByteArray blob0, qsizetype uid, qsizetype hid, int extension0) 
	: MediaHandlingData(blob0, hid, extension0) , uploadId(uid)
{}
MediaUploadingData::MediaUploadingData(MediaHandlingData data, int uid) : MediaHandlingData(std::move(data)) , uploadId(uid)
{}

MediaUploadingData::MediaUploadingData(qsizetype id0, int extension0) : uploadId(id0) 
{
	extension = extension0;
}
MediaUploadingData::MediaUploadingData(MediaHandlingData data) : MediaHandlingData(std::move(data)) , uploadId(0)
{}



MediaDataHandlingInfo::MediaDataHandlingInfo()
	:ready(false) , uploaded(false)
{}

MediaDataHandlingInfo::MediaDataHandlingInfo(qsizetype uid , qsizetype hid , bool ready0, bool uploaded0)
	:uploadId(uid) , handlingId(hid), ready(ready0), uploaded(uploaded0)
{}

