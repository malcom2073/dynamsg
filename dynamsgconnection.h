#ifndef DynaMsgCONNECTION_H
#define DynaMsgCONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <dynamsgparser.h>

class DynaMsgConnection : public QObject
{
	Q_OBJECT
public:
	explicit DynaMsgConnection(QObject *parent = 0);
	DynaMsgConnection(QTcpSocket *socket,QObject *parent);
	void listen(int port);
	void connectToHost(QString hostaddress, int port);
	void sendPayloadToWire(QByteArray payload);
	void sendAuthRequest(QString name);
	DynaMsgParser *getParser() { return m_parser; }
	void sendAuthReply();
	void setRemoteId(quint64 remoteid) { m_remoteId = remoteid; m_parser->setRemoteId(m_remoteId); }
	void setLocalId(quint64 localid) { m_localId = localid; m_parser->setLocalId(m_localId); }
	void sendSubscribeRequesst(QString topic);
	void sendSubscribedMessage(QString messageName,QByteArray content);
	void sendOpenPortRequest(quint64 sender);
private:
	quint64 m_remoteId;
	quint64 m_localId;
	QTcpServer *m_server;
	QTcpSocket *m_socket;
	bool m_authed;
	QByteArray m_socketBuffer;
	QList<QByteArray> m_packetBuffer;
	void checkBuffer();
	DynaMsgParser *m_parser;
signals:
	void connected();
	void authRequest(quint64 target, quint64 sender,QJsonObject authobject);
	void authResponse(quint64 target, quint64 sender,QJsonObject authobject);
	void subscribeRequest(quint64 target, quint64 sender,QString name);
	void incomingSubscribedMessage(QString name,QByteArray payload);
	void incomingPublishMessage(quint64 sender,QString name,QByteArray payload);
	void incomingPortOpenRequest(quint64 requester);
public slots:
	void socketReadyRead();
	void socketDisconnected();
	void serverNewConnection();
};

#endif // DynaMsgCONNECTION_H
