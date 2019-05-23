#include "dynamsgconnection.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

DynaMsgConnection::DynaMsgConnection(QObject *parent) : QObject(parent)
{
	m_server = 0;
	m_socket = 0;
	m_localId = 0;
	m_remoteId = 0;
	m_authed = false;
	m_parser = new DynaMsgParser(this);
	//connect(m_parser,SIGNAL(jsonPacketReceived(QJsonObject)),this,SIGNAL(si_jsonPacketReceived(QJsonObject)));
	//connect(m_parser,SIGNAL(publishMessage(QString,QByteArray)),this,SIGNAL(si_publishMessage(QString,QByteArray)));
	connect(m_parser,SIGNAL(subscribeMessage(quint64,quint64,QString)),this,SIGNAL(subscribeRequest(quint64,quint64,QString)));
	//connect(m_parser,SIGNAL(ptpMessageReceived(QString,QString,QByteArray)),this,SIGNAL(si_ptpMessageReceived(QString,QString,QByteArray)));
	connect(m_parser,SIGNAL(authRequest(quint64,quint64,QJsonObject)),this,SIGNAL(authRequest(quint64,quint64,QJsonObject)));
	connect(m_parser,SIGNAL(authResponse(quint64,quint64,QJsonObject)),this,SIGNAL(authResponse(quint64,quint64,QJsonObject)));
	connect(m_parser,SIGNAL(incomingSubscribedMessage(QString,QByteArray)),this,SIGNAL(incomingSubscribedMessage(QString,QByteArray)));
	connect(m_parser,SIGNAL(incomingPublishMessage(quint64,QString,QByteArray)),this,SIGNAL(incomingPublishMessage(quint64,QString,QByteArray)));
	connect(m_parser,SIGNAL(incomingPortOpenRequest(quint64)),this,SIGNAL(incomingPortOpenRequest(quint64)));

}
DynaMsgConnection::DynaMsgConnection(QTcpSocket *socket,QObject *parent) : QObject(parent)
{
	m_server = 0;
	m_socket = socket;
	connect(m_socket,SIGNAL(disconnected()),this,SLOT(socketDisconnected()));
	connect(m_socket,SIGNAL(readyRead()),this,SLOT(socketReadyRead()));
	connect(m_socket,SIGNAL(connected()),this,SIGNAL(connected()));
	m_authed = false;
	m_parser = new DynaMsgParser(this);
	//connect(m_parser,SIGNAL(jsonPacketReceived(QJsonObject)),this,SIGNAL(si_jsonPacketReceived(QJsonObject)));
	//connect(m_parser,SIGNAL(publishMessage(QString,QByteArray)),this,SIGNAL(si_publishMessage(QString,QByteArray)));
	connect(m_parser,SIGNAL(subscribeMessage(quint64,quint64,QString)),this,SIGNAL(subscribeRequest(quint64,quint64,QString)));
	//connect(m_parser,SIGNAL(ptpMessageReceived(QString,QString,QByteArray)),this,SIGNAL(si_ptpMessageReceived(QString,QString,QByteArray)));
	connect(m_parser,SIGNAL(authRequest(quint64,quint64,QJsonObject)),this,SIGNAL(authRequest(quint64,quint64,QJsonObject)));
	connect(m_parser,SIGNAL(authResponse(quint64,quint64,QJsonObject)),this,SIGNAL(authResponse(quint64,quint64,QJsonObject)));
	connect(m_parser,SIGNAL(incomingSubscribedMessage(QString,QByteArray)),this,SIGNAL(incomingSubscribedMessage(QString,QByteArray)));
	connect(m_parser,SIGNAL(incomingPublishMessage(quint64,QString,QByteArray)),this,SIGNAL(incomingPublishMessage(quint64,QString,QByteArray)));
	connect(m_parser,SIGNAL(incomingPortOpenRequest(quint64)),this,SIGNAL(incomingPortOpenRequest(quint64)));
}

void DynaMsgConnection::connectToHost(QString hostaddress, int port)
{
	m_socket = new QTcpSocket(this);
	connect(m_socket,SIGNAL(disconnected()),this,SLOT(socketDisconnected()));
	connect(m_socket,SIGNAL(readyRead()),this,SLOT(socketReadyRead()));
	connect(m_socket,SIGNAL(connected()),this,SIGNAL(connected()));
	m_socket->connectToHost(hostaddress,port);
}

void DynaMsgConnection::sendPayloadToWire(QByteArray payload)
{

}



void DynaMsgConnection::sendAuthRequest(QString name)
{
	m_socket->write(m_parser->generateAuthRequest(name));
	m_socket->flush();
}
void DynaMsgConnection::sendAuthReply()
{
	m_socket->write(m_parser->generateAuthReply(0));
	m_socket->flush();

}

void DynaMsgConnection::listen(int port)
{
	m_server = new QTcpServer(this);
	connect(m_server,SIGNAL(newConnection()),this,SLOT(serverNewConnection()));
	m_server->listen(QHostAddress::LocalHost,port);
}

void DynaMsgConnection::socketDisconnected()
{
	m_server->deleteLater();
}
void DynaMsgConnection::socketReadyRead()
{
	qDebug() << "Ready Read";
	m_socketBuffer.append(m_socket->readAll());
	checkBuffer();
}
void DynaMsgConnection::sendSubscribeRequesst(QString topic)
{
	m_socket->write(m_parser->generateSubscribeRequest(topic));
	m_socket->flush();
}
void DynaMsgConnection::sendSubscribedMessage(QString messageName,QByteArray content)
{
	m_socket->write(m_parser->generateSubscribedMessage(messageName,content));
	m_socket->flush();
}
void DynaMsgConnection::sendOpenPortRequest(quint64 sender)
{
	m_socket->write(m_parser->generateOpenPortRequest(sender));
	m_socket->flush();
}

void DynaMsgConnection::checkBuffer()
{
	if (m_packetBuffer.size() > 0)
	{
		QByteArray buf = m_packetBuffer.at(0);
		m_packetBuffer.removeAt(0);
		m_parser->parsePacket(buf);
	}
	if (m_socketBuffer.size() <= 11)
	{
		//Not large enough for auth
		qDebug() << "Not enough for auth:" << m_socketBuffer.size();
		return;
	}
	if (m_socketBuffer.at(0) == 0x01 && m_socketBuffer.at(1) == 0x02 && m_socketBuffer.at(2) == 0x03)
	{
		//Start byte! Read length
		quint32 length = 0;
		length += ((unsigned char)m_socketBuffer.at(3)) << 24;
		length += ((unsigned char)m_socketBuffer.at(4)) << 16;
		length += ((unsigned char)m_socketBuffer.at(5)) << 8;
		length += ((unsigned char)m_socketBuffer.at(6)) << 0;
		qDebug() << "Length:" << length;
		if (m_socketBuffer.size() >= length+11)
		{
			//We have a full packet! Should be an auth packet, so download and verify!
			//qDebug() << "Buffer size before:" << m_socketBuffer.size();
			QByteArray packet = m_socketBuffer.mid(7,length);
			//qDebug() << "JSON:" << packet;
			m_socketBuffer.remove(0,length+11);
			m_packetBuffer.append(packet);
			//qDebug() << "Buffer size after:" << m_socketBuffer.size();
			checkBuffer();
			return;
		}
		else
		{
			qDebug() << "Bad length";
			return;
		}
	}
	qDebug() << "Bad packet:" << m_socketBuffer.toHex();
	return;
}

void DynaMsgConnection::serverNewConnection()
{
	m_socket = m_server->nextPendingConnection();
	if (!m_socket)
	{
		return;
	}
	m_server->close();
	connect(m_socket,SIGNAL(disconnected()),this,SLOT(socketDisconnected()));
	connect(m_socket,SIGNAL(readyRead()),this,SLOT(socketReadyRead()));
}
