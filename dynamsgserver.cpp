#include "dynamsgserver.h"

DynaMsgServer::DynaMsgServer(QObject *parent) : QObject(parent)
{
	m_server = new QTcpServer(this);
	connect(m_server,SIGNAL(newConnection()),this,SLOT(serverNewConnection()));

}
void DynaMsgServer::listen(int port)
{
	m_server->listen(QHostAddress::LocalHost,port);
}

void DynaMsgServer::serverNewConnection()
{
	QTcpSocket *socket = m_server->nextPendingConnection();
	DynaMsgConnection *connection = new DynaMsgConnection(socket,this);
	m_pendingConnections.append(connection);
	emit newClientConnection();
}
DynaMsgConnection *DynaMsgServer::nextConnection()
{
	if (m_pendingConnections.size() > 0)
	{
		DynaMsgConnection *connection = m_pendingConnections.at(0);
		m_pendingConnections.removeAt(0);
		return connection;
	}
	return 0;
}
