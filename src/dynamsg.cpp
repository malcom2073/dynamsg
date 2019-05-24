#include "dynamsg.h"
#include <QDateTime>
#include <QTcpSocket>
#include <QTcpServer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "dynamsgparser.h"
#include <limits>
#include <random>
DynaMsg::DynaMsg(QString key, QObject *parent) : QObject(parent)
{
	m_key = key;
	m_ipcDataStore = new DynaMsgDataStore();
	m_coreConnection = 0;
}
void DynaMsg::setName(QString key)
{
	m_key = key;
}


void DynaMsg::connectToCore(QString address, int portNum)
{
	m_coreConnection = new DynaMsgConnection(this);
	connect(m_coreConnection,SIGNAL(connected()),this,SLOT(coreConnectionConnected()));
	connect(m_coreConnection,SIGNAL(authResponse(quint64,quint64,QJsonObject)),this,SLOT(authResponse(quint64,quint64,QJsonObject)));
	connect(m_coreConnection,SIGNAL(incomingSubscribedMessage(QString,QByteArray)),this,SIGNAL(incomingSubscribedMessage(QString,QByteArray)));
	connect(m_coreConnection,SIGNAL(incomingPortOpenRequest(quint64)),this,SLOT(incomingPortOpenRequest(quint64)));
	connect(m_coreConnection,SIGNAL(si_ptpMessageReceived(quint64,quint64,QString,QByteArray)),this,SLOT(ptpMessageReceived(quint64,quint64,QString,QByteArray)));

	m_coreConnection->connectToHost(address,portNum);
}
void DynaMsg::setConnectionPass(QString pass)
{
	m_pass = pass;
}

void DynaMsg::disconnectFromCore()
{
	m_coreConnection->disconnectFromHost();
}
void DynaMsg::ptpMessageReceived(quint64 targetid, quint64 senderid,QString target, QByteArray payload)
{
	if (m_authedConnectionMap.contains(target))
	{
		qDebug() << "DynaMsg: Got message for someone else, passing it on:" << payload;
		m_authedConnectionMap.value(target)->sendPtpMessage(target,payload);
		return;
	}
	if (m_connectionIdToConnectionMap.contains(senderid))
	{
		if (m_connectionIdToConnectionMap.value(senderid)->getLocalId() == targetid)
		{
			qDebug() << "DynaMsg: Got message for me:" << payload.toHex();
			emit si_incomingMessage(payload);
		}
		else
		{
			for (int i=0;i<m_authedConnectionList.size();i++)
			{
				if (targetid == m_authedConnectionList.at(i)->getRemoteId())
				{
					//Pass the message on!
					qDebug() << "DynaMsg: Got message for someone else, passing it on:" << payload;
					m_authedConnectionList.at(i)->sendPtpMessage(target,payload);
					return;
				}
			}
			qDebug() << "DynaMsg: Got message for someone else:" << payload;
		}

	}
	else
	{
		if (m_coreConnection->getLocalId() == targetid)
		{
			qDebug() << "DynaMsg: Got message for me:" << payload;
			emit si_incomingMessage(payload);
		}
		else
		{
			qDebug() << "DynaMsg: Got message for someone else:" << payload;
		}
	}
}

void DynaMsg::coreConnectionConnected()
{
	//Send auth!
	qDebug() << "Connected to core";
	m_coreConnection->sendAuthRequest(m_key,m_pass);
}
void DynaMsg::startListen(int port)
{
	m_server = new DynaMsgServer(this);
	connect(m_server,SIGNAL(newClientConnection()),this,SLOT(newClientConnected()));
	m_server->listen(port);
}
void DynaMsg::newClientConnected()
{
	qDebug() << "new Client connected";
	DynaMsgConnection *connection = m_server->nextConnection();
	connect(connection,SIGNAL(authRequest(quint64,quint64,QJsonObject)),this,SLOT(authRequest(quint64,quint64,QJsonObject)));
	connect(connection,SIGNAL(subscribeRequest(quint64,quint64,QString)),this,SIGNAL(subscribeRequest(quint64,quint64,QString)));
	connect(connection,SIGNAL(incomingPublishMessage(quint64,QString,QByteArray)),this,SIGNAL(incomingPublishMessage(quint64,QString,QByteArray)));
	connect(connection,SIGNAL(si_ptpMessageReceived(quint64,quint64,QString,QByteArray)),this,SLOT(ptpMessageReceived(quint64,quint64,QString,QByteArray)));
	m_unAuthedConnectionList.append(connection);
}
quint64 DynaMsg::getRandomId()
{
	std::default_random_engine generator;
	std::uniform_int_distribution<quint64> distribution(1,std::numeric_limits<quint64>::max());
	quint64 dice_roll = distribution(generator);  // generates number in the range 1..6
	while (m_connectionIdList.contains(dice_roll))
	{
		dice_roll = distribution(generator);
	}
	m_connectionIdList.append(dice_roll);
	return dice_roll;

}

void DynaMsg::authRequest(quint64 targetid, quint64 senderid, QJsonObject authobject)
{
//Auth is requested!
	qDebug() << "Auth request from:" << senderid << " to " << targetid;
	DynaMsgConnection *connection = qobject_cast<DynaMsgConnection*>(sender());
	if (!connection)
	{
		return;
	}
	if (!m_unAuthedConnectionList.contains(connection))
	{
		qDebug() << "Unauthorized connection not in list??";
		return;
	}
	if (authobject.value("pass").toString() == "" || authobject.value("pass").toString() == "pass")
	{
		m_unAuthedConnectionList.removeOne(connection);
		m_authedConnectionList.append(connection);
		if (senderid == 0x0)
		{
			//Sender has no ID, generate one for them
			quint64 connectionid = getRandomId();
			//m_connectionIdToNameMap.insert(connectionid,authobject.value("name").toString());
			m_connectionIdToConnectionMap.insert(connectionid,connection);
			qDebug() << "Connection" << connectionid << authobject.value("name").toString();
			connection->setLocalId(targetid);
			connection->setRemoteId(connectionid);
			m_authedConnectionMap.insert(authobject.value("name").toString(),connection);
		}
		else
		{
			connection->setRemoteId(senderid);
			connection->setLocalId(targetid);
		}
		qDebug() << "Sending auth reply";
		connection->sendAuthReply(true);
		emit clientConnected();
	}
	else
	{
		qDebug() << "Invalid auth";
		m_unAuthedConnectionList.removeOne(connection);
		connection->sendAuthReply(false);

		connection->disconnectFromHost();
	}
}
void DynaMsg::authResponse(quint64 target, quint64 sender, QJsonObject caps)
{
	if (caps.value("auth") == "successfull")
	{
		//We're good!
	}
	else if (caps.value("auth") == "failure")
	{
		emit coreRejected();
		return;
	}


	//Target is our ID, cache it for later.
	m_coreConnection->setLocalId(target);
	m_coreConnection->setRemoteId(sender);
	qDebug() << "Auth response:" << target << sender << QJsonDocument(caps).toJson();
	emit coreConnected();
}

void DynaMsg::connectToHost(QString address, int portNum)
{
	connectToCore(address,portNum);
}


void DynaMsg::sendJsonMessage(QString target, QJsonObject object)
{
	QByteArray jsonpacket = makeJsonPacket(object);
//	m_socket->write(jsonpacket);
}
QByteArray DynaMsg::makeJsonPacket(QJsonObject message)
{
	QJsonDocument doc(message);
	QByteArray jsonbytes = doc.toBinaryData();


	QByteArray retval;
	//Header
	retval.append(0x01);
	retval.append(0x02);
	retval.append(0x03);

	//Length
	retval.append(((unsigned char)(jsonbytes.length() >> 24)) & 0xFF);
	retval.append(((unsigned char)(jsonbytes.length() >> 16)) & 0xFF);
	retval.append(((unsigned char)(jsonbytes.length() >> 8)) & 0xFF);
	retval.append(((unsigned char)(jsonbytes.length() >> 0)) & 0xFF);


	//Message type, JSON
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x02);

	//Checksum
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);

	return retval;
}
void DynaMsg::sendSubscribedMessage(quint64 target,QString messageName,QByteArray content)
{
	if (!m_connectionIdToConnectionMap.contains(target))
	{
		qDebug() << "Attempting to send a subscription message to" << target << "but it does not exist in m_connectionIdToConnectionMap";
		return;
	}
	m_connectionIdToConnectionMap[target]->sendSubscribedMessage(messageName,content);
}
void DynaMsg::incomingPortOpenRequest(quint64 requester)
{
	qDebug() << "Port open requested by:" << requester;

}

void DynaMsg::sendOpenPortRequest(quint64 target,quint64 sender,quint16 port)
{
	if (!m_connectionIdToConnectionMap.contains(target))
	{
		qDebug() << "Attempting to send a open port request to" << target << "but it does not exist in m_connectionIdToConnectionMap";
		return;
	}
//	m_connectionIdToConnectionMap[target]->sendOpenPortRequest(target,sender,port);
}

void DynaMsg::publishMessage(QString messageName,QByteArray content)
{
	QByteArray message = generatePublishMessage(messageName,content);
	QByteArray packet = generateCorePacket(message);
//	m_socket->write(packet);
}
void DynaMsg::sendMessage(QString target,QByteArray content,QString sender)
{
	if (sender == "")
	{
		sender = m_key;
	}
	if (m_authedConnectionMap.contains(target))
	{
		m_authedConnectionMap.value(target)->sendPtpMessage(target,content);
	}
	else
	{
		m_coreConnection->sendPtpMessage(target,content);
	}
//	QByteArray message = generateSendMessage(target,sender,content);
//	QByteArray packet = generateCorePacket(message);
//	m_socket->write(packet);
}
QByteArray DynaMsg::generateSendMessage(QString target,QString sender,QByteArray payload)
{
	QByteArray retval;

	//Message type, PTP message
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x0B);

	//Message Flags
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);

	//Length Of Target
	retval.append(((unsigned char)(target.length() >> 24)) & 0xFF);
	retval.append(((unsigned char)(target.length() >> 16)) & 0xFF);
	retval.append(((unsigned char)(target.length() >> 8)) & 0xFF);
	retval.append(((unsigned char)(target.length() >> 0)) & 0xFF);

	retval.append(target);


	//Length Of sender
	retval.append(((unsigned char)(sender.length() >> 24)) & 0xFF);
	retval.append(((unsigned char)(sender.length() >> 16)) & 0xFF);
	retval.append(((unsigned char)(sender.length() >> 8)) & 0xFF);
	retval.append(((unsigned char)(sender.length() >> 0)) & 0xFF);

	retval.append(sender);



	retval.append(payload);

	return retval;
}

void DynaMsg::subscribeMessage(QString messageName)
{
	//Send a message to the core, asking to subscribe to this message.
//	QByteArray message = generateSubscribeMessage(messageName);
//	QByteArray packet = generateCorePacket(message);
//	m_socket->write(packet);
	m_coreConnection->sendSubscribeRequesst(messageName);
}
/*void DynaMsg::subscribeRequest(quint64 target, quint64 sender,QString name)
{
	//This is a core only message, pass it up to the core to figure out.
}*/

QByteArray DynaMsg::generateSubscribeMessage(QString messageName)
{
	QJsonObject messageobj;
	messageobj.insert("name",messageName);
	QJsonDocument doc(messageobj);

	QByteArray retval;

	//Message type, subscribe request
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x03);

	//Message Flags
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);

	//Targetlen
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);

	//senderlen
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);



	retval.append(doc.toJson());

	return retval;
}
QByteArray DynaMsg::generateAuthMessage(QString key)
{
//QString package = "{\"type\":\"auth\",\"key\":\"" + m_key + "\"}";
	QJsonObject messageobj;
	messageobj.insert("type","auth");
	messageobj.insert("key",key);
	QJsonDocument doc(messageobj);
	QByteArray retval;

	//Message type, auth request
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x01);

	//No message flags

	retval.append(doc.toJson());
	return retval;
}

QByteArray DynaMsg::generatePublishMessage(QString messageName,QByteArray payload)
{
	QJsonObject messageobj;
	messageobj.insert("name",messageName);
	messageobj.insert("payload",QJsonValue::fromVariant(QVariant::fromValue(payload)));
	QJsonDocument doc(messageobj);

	QByteArray retval;

	//Message type, subscribe request
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x07);


	//Message Flags
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);

	//Targetlen
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);

	//senderlen
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);




	retval.append(doc.toJson());

	return retval;
}
\

QByteArray DynaMsg::generateCorePacket(QByteArray messageBytes)
{
	QByteArray retval;
	//Header
	retval.append(0x01);
	retval.append(0x02);
	retval.append(0x03);

	//Length
	retval.append(((unsigned char)(messageBytes.length() >> 24)) & 0xFF);
	retval.append(((unsigned char)(messageBytes.length() >> 16)) & 0xFF);
	retval.append(((unsigned char)(messageBytes.length() >> 8)) & 0xFF);
	retval.append(((unsigned char)(messageBytes.length() >> 0)) & 0xFF);

	//Message
	retval.append(messageBytes);

	//Checksum
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);
	retval.append((char)0x00);

	return retval;
}


