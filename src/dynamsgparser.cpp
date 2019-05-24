#include "dynamsgparser.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QDebug>
#define AUTH_REQUEST 0x1
#define AUTH_RESPONSE 0x2
#define SUBSCRIBE_REQUEST 0x3
#define SUBSCRIBE_MESSAGE 0x4
#define PUBLISH_MESSAGE 0x5
#define PORT_REQUEST 0x6
DynaMsgParser::DynaMsgParser(QObject *parent) : QObject(parent)
{
	m_remoteId = 0;
	m_localId = 0;
}

QByteArray DynaMsgParser::generateAuthRequest(QString name,QString pass)
{
	QJsonObject authobject;
	authobject.insert("type","auth");
	authobject.insert("name",name);
	authobject.insert("pass",pass);
	QJsonDocument doc(authobject);
	return generateWire(generateMessage(doc.toJson(),AUTH_REQUEST));

}
QByteArray DynaMsgParser::generateAuthReply(bool valid,quint64 id)
{
	QJsonObject authreply;
	if (valid)
	{
		authreply.insert("auth","successful");
	}
	else
	{
		authreply.insert("auth","failure");
	}
	return generateWire(generateMessage(QJsonDocument(authreply).toJson(),AUTH_RESPONSE));
}
QByteArray DynaMsgParser::generateSubscribeRequest(QString name)
{
	QJsonObject subscriberequest;
	subscriberequest.insert("name",name);
	return generateWire(generateMessage(QJsonDocument(subscriberequest).toJson(),SUBSCRIBE_REQUEST));
}
QByteArray DynaMsgParser::generateSubscribedMessage(QString name,QByteArray content)
{
	QByteArray message;
	message.append(quint32ToBytes(name.length()));
	message.append(name.toLatin1());
	message.append(content);
	return generateWire(generateMessage(message,SUBSCRIBE_MESSAGE));
}
QByteArray DynaMsgParser::generateOpenPortRequest(quint64 sender)
{
	QByteArray message;
	message.append(quint64ToBytes(sender));
	return generateWire(generateMessage(message,PORT_REQUEST));
}

QByteArray DynaMsgParser::generateWire(QByteArray payload)
{
	QByteArray retval;
	//Header
	retval.append(0x01);
	retval.append(0x02);
	retval.append(0x03);

	//Length
	retval.append(quint32ToBytes(payload.length()));

	//Message
	retval.append(payload);

	//Checksum
	retval.append(quint32ToBytes(0));
	return retval;
}
QByteArray DynaMsgParser::quint32ToBytes(quint32 value)
{
	QByteArray retval;
	retval.append(((unsigned char)(value >> 24)) & 0xFF);
	retval.append(((unsigned char)(value >> 16)) & 0xFF);
	retval.append(((unsigned char)(value >> 8)) & 0xFF);
	retval.append(((unsigned char)(value >> 0)) & 0xFF);
	return retval;
}
QByteArray DynaMsgParser::quint64ToBytes(quint64 value)
{
	QByteArray retval;
	retval.append(((unsigned char)(value >> 56)) & 0xFF);
	retval.append(((unsigned char)(value >> 48)) & 0xFF);
	retval.append(((unsigned char)(value >> 40)) & 0xFF);
	retval.append(((unsigned char)(value >> 32)) & 0xFF);
	retval.append(((unsigned char)(value >> 24)) & 0xFF);
	retval.append(((unsigned char)(value >> 16)) & 0xFF);
	retval.append(((unsigned char)(value >> 8)) & 0xFF);
	retval.append(((unsigned char)(value >> 0)) & 0xFF);
	return retval;
}

QByteArray DynaMsgParser::generateMessage(QByteArray message, quint32 type)
{
	QByteArray retval;
	retval.append(quint32ToBytes(type));
	retval.append(quint64ToBytes(m_remoteId));
	retval.append(quint64ToBytes(m_localId));
	retval.append(message);
	return retval;
}
quint32 DynaMsgParser::bytesToquint32(QByteArray bytes)
{
	quint32 retval = 0;
	retval += ((unsigned char)bytes.at(0)) << 24;
	retval += ((unsigned char)bytes.at(1)) << 16;
	retval += ((unsigned char)bytes.at(2)) << 8;
	retval += ((unsigned char)bytes.at(3)) << 0;
	return retval;
}

quint64 DynaMsgParser::bytesToquint64(QByteArray bytes)
{
	quint64 retval = 0;
	retval += ((quint64)((unsigned char)bytes.at(0))) << 56;
	retval += ((quint64)((unsigned char)bytes.at(1))) << 48;
	retval += ((quint64)((unsigned char)bytes.at(2))) << 40;
	retval += ((quint64)((unsigned char)bytes.at(3))) << 32;
	retval += ((quint64)((unsigned char)bytes.at(4))) << 24;
	retval += ((quint64)((unsigned char)bytes.at(5))) << 16;
	retval += ((quint64)((unsigned char)bytes.at(6))) << 8;
	retval += ((quint64)((unsigned char)bytes.at(7))) << 0;
	return retval;

}

bool DynaMsgParser::parsePacket(const QByteArray & packet)
{
	quint32 type = 	bytesToquint32(packet.mid(0,4));
	quint64 targetid = bytesToquint64(packet.mid(4,8));
	quint64 senderid = bytesToquint64(packet.mid(12,8));
	qDebug() << "ParsePacket:" << QString::number(type,16);
	if (type == AUTH_REQUEST)
	{
		//Auth message
		//First 8 are ignored on auth message
		QJsonDocument doc = QJsonDocument::fromJson(packet.mid(20));
		QJsonObject topobject = doc.object();
//		emit jsonPacketReceived(topobject);
		emit authRequest(targetid,senderid,topobject);
	}
	else if (type == AUTH_RESPONSE)
	{
		//Auth response from the other party, will contain capabilities of the connected client, or a rejection.
		QJsonDocument doc = QJsonDocument::fromJson(packet.mid(20));
		QJsonObject topobject = doc.object();
		emit authResponse(targetid,senderid,topobject);
	}
	else if (type == SUBSCRIBE_REQUEST)
	{
		QJsonDocument doc = QJsonDocument::fromJson(packet.mid(20));
		QJsonObject subobj = doc.object();
		QString subname = subobj.value("name").toString();
		emit subscribeMessage(targetid, senderid,subname);
	}
	else if (type == SUBSCRIBE_MESSAGE)
	{
		quint32 namelength = bytesToquint32(packet.mid(20));
		QString messagename = packet.mid(24,namelength);
		QByteArray message = packet.mid(24+namelength);
		qDebug() << "Message for:" << messagename;
		emit incomingSubscribedMessage(messagename,message);
	}
	else if (type == PUBLISH_MESSAGE)
	{
		quint32 namelength = bytesToquint32(packet.mid(20));
		QString messagename = packet.mid(24,namelength);
		QByteArray message = packet.mid(24+namelength);
		qDebug() << "Publish message:" << messagename;
		emit incomingPublishMessage(senderid,messagename,message);
	}
	else if (type == PORT_REQUEST)
	{
		quint64 requesterid = bytesToquint64(packet.mid(20));
		emit incomingPortOpenRequest(requesterid);
	}
	else if (type == 7)
	{
		//Subscribe
		QJsonDocument doc = QJsonDocument::fromJson(packet.mid(16));
		QJsonObject pubobj = doc.object();
		QString pubname = pubobj.value("name").toString();
		QByteArray pubmsg = pubobj.value("payload").toVariant().toByteArray();
		emit publishMessage(pubname,pubmsg);
	}
	else if (type == 0x0B)
	{
		//PTP message
		quint32 targetlen = 0;
		targetlen += ((unsigned char)packet.at(8)) << 24;
		targetlen += ((unsigned char)packet.at(9)) << 16;
		targetlen += ((unsigned char)packet.at(10)) << 8;
		targetlen += ((unsigned char)packet.at(11)) << 0;
		QString targetstr = packet.mid(12,targetlen);

		quint32 senderlen = 0;
		senderlen += ((unsigned char)packet.at(12+targetlen)) << 24;
		senderlen += ((unsigned char)packet.at(13+targetlen)) << 16;
		senderlen += ((unsigned char)packet.at(14+targetlen)) << 8;
		senderlen += ((unsigned char)packet.at(15+targetlen)) << 0;
		QString senderstr = packet.mid(16+targetlen,senderlen);
		QByteArray payload = packet.mid(16+targetlen+senderlen);
		emit ptpMessageReceived(targetstr,senderstr,payload);

	}
	else if (type == 2)
	{
		//JSON
		//return parseJsonPacket(packet.mid(4));
		QJsonDocument doc = QJsonDocument::fromJson(packet.mid(16));
		QJsonObject topobject = doc.object();
		emit jsonPacketReceived(topobject);

		return true;
	}
	else
	{
		qDebug() << "Unknown type returned:" << type;
	}
	return false;

}
bool DynaMsgParser::parseJsonPacket(const QByteArray &packet)
{
	QJsonDocument doc = QJsonDocument::fromJson(packet);
	QJsonObject topobject = doc.object();
	return true;

}
