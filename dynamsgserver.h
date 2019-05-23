#ifndef DynaMsgServer_H
#define DynaMsgServer_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include "dynamsgconnection.h"

class DynaMsgServer : public QObject
{
	Q_OBJECT
public:
	explicit DynaMsgServer(QObject *parent = 0);
	DynaMsgConnection *nextConnection();
	void listen(int port);
private:
	QTcpServer *m_server;
	QList<DynaMsgConnection*> m_pendingConnections;
signals:
	void newClientConnection();
private slots:
	void serverNewConnection();
public slots:
};

#endif // DynaMsgServer_H
