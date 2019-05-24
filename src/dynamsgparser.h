#ifndef DYNAMSGPARSER_H
#define DYNAMSGPARSER_H

#include <QObject>
#include <QJsonObject>

class DynaMsgParser : public QObject
{
	Q_OBJECT
public:
	explicit DynaMsgParser(QObject *parent = 0);
	bool parsePacket(const QByteArray & packet);
	bool parseJsonPacket(const QByteArray &packet);
	QByteArray generateAuthRequest(QString name,QString pass);
	QByteArray generateWire(QByteArray payload);
	QByteArray generateMessage(QByteArray message, quint32 type);
	QByteArray generateSubscribeRequest(QString name);
	QByteArray generateSubscribedMessage(QString name,QByteArray content);
	QByteArray generateOpenPortRequest(quint64 sender);
	QByteArray quint32ToBytes(quint32 value);
	QByteArray quint64ToBytes(quint64 value);
	quint32 bytesToquint32(QByteArray bytes);
	quint64 bytesToquint64(QByteArray bytes);
	QByteArray generateAuthReply(bool valid,quint64 id);
	void setRemoteId(quint64 remoteid) { m_remoteId = remoteid; }
	void setLocalId(quint64 localid) { m_localId = localid; }
private:
	quint64 m_remoteId;
	quint64 m_localId;

signals:
	void incomingSubscribedMessage(QString name,QByteArray payload);
	void jsonPacketReceived(QJsonObject message);
	void subscribeMessage(quint64 target, quint64 sender,QString message);
	void publishMessage(QString name, QByteArray payload);
	void ptpMessageReceived(QString target,QString sender,QByteArray payload);
	void authResponse(quint64 target, quint64 sender, QJsonObject caps);
	void authRequest(quint64 target, quint64 sender, QJsonObject authobject);
	void incomingPublishMessage(quint64 sender,QString name,QByteArray payload);
	void incomingPortOpenRequest(quint64 requester);
public slots:
};

#endif // DYNAMSGPARSER_H
