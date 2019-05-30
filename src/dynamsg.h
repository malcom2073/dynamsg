#ifndef DynaMsg_H
#define DynaMsg_H



/*

DynaMsgParser - Parses payloads
DynaMsgConnection handles a single connection, and wire structure. Owns the parser
DynaMsg Handles auth, sending/receiving. Will have multiple DynaMsgconnections, but only one DynaMsg. Owns the DynaMsgConnection

Application creates a DynaMsg. Connects to the server (new DynaMsgconnection/Parser)
DynaMsg send an auth request to join the server, gets a client ID back.
Application can now send/receieve, publish/subscribe.

Core creates DynaMsg, tells it to accept connections. Incoming connections get authed, parsed messages go up to core.

core handles subscribe requests

Subscribe is what a client does when it wants to receive publish events for a message
Publish is what a client does whne it wants to send out a message to all subscribers
Send Message sends a message to a specific client
Unsubscribe removes a client from the subscription list of a message.

Wire structure, checksum is not included for TCP/UDP, header and length are not included for UDP
Header and length are missing for UDP
| Header |  Length   |  Payload  | Checksum  |
|01|02|03|AA|BB|CC|DD|B1|B2|B3|BN|EE|FF|GG|HH|

Payload structure:
|   Type    |        Target  ID     |       Sender ID       |  Message  |
|T1|T2|T3|T4|B1|B2|B3|B4|B5|B6|B7|B8|B1|B2|B3|B4|B5|B6|B7|B8|B1|B2|B3|BN|

Each client is identical. Clients can connect to other clients, and tell them: What clients are connected to them,
and also what services they provide

Payload types:
|00|00|00|01| - Auth packet, this comes from the client immediatly upon connecting
TargetID = 0x0 - No target if client initially connecting to core.
SenderID = 0x0 if a client initially connecting to core,
Message = JSON of auth structure
{ "type" : "auth",
"key" : CLIENT_NAME,
"caps" : [
List of capabilities here
]
}
Server responds with
|00|00|00|02|
TargetID = Client targetID
SenderID = 0x0
Message = JSON
{
"caps" : [
List of server capabilities here
]
}



|00|00|00|03| - Subscribe. Sent from client to core, requesting to subscribe.
flags
|00|00|00|00|
Target ID
|00|00|00|00|00|00|00|00|
Sender ID
Client that send the subscribe request
Message
Text string of the message to subscribe to

core keeps a list of publishers. Core checks the publisher, to ensure connection types match the subscriber.
If so, it sends a message to the subscriber telling to to open the port.
Preference:
TCP before UDP
Provider -> Subscriber before subscriber->provider
Core -> Provider/subscriber before provider/subscriber -> Core

|00|00|00|04| - Core telling the subscriber to open a port
Subscriber opens the port as appropriate.
|00|00|00|05| - Subscriber telling core which port is open
|00|00|00|06| - Core telling publisher to send to subscriber
Publisher connects to subscriber on the port, and commences sending.
|00|00|00|07| - Publisher telling core it was succesful.
OR
|00|00|00|08| - Core telling the publisher to open a port
publisher opens the port as appropriate.
|00|00|00|09| - publisher telling core which port is open
|00|00|00|0A| - Core telling subscriber to open a port to publisher
subscriber connects to publisher on the port
|00|00|00|0B| - subscriber telling core it was succesful.
OR
Core opens a port for the subscriber
|00|00|00|0C| - Core telling the subscriber to connect to which port
Core opens a port for the publisher
|00|00|00|0D| - Core telling the publisher to connect to which port
Core starts forwarding messages
OR
|00|00|00|0E| - Core telling subscriber to open a port
|00|00|00|0F| - subscriber replies with the port number
Core connects to the port
|00|00|00|10| - Core telling the publisher to open a port
|00|00|00|11| - publisher replies with the port number
Core connects to the port
Core forwards traffic

Any combonation thereof, depending on capabilities.

Publishers and subscribers can define capabilities:
CAN_RECIEVE_UDP
List incoming UDP ports
CAN_RECEIVE_TCP
List incoming tcp ports
CAN_SEND_UDP
List outgoing udp prots
CAN_SEND_TCP
List outgoing tcp ports

If none are defined, the core assumes only outbound connections from clients are available.

TargetLen and senderlen are always included, but if zero there is no target or sender.
Only type 0x0000000B has a target, since is a PTP directed message. Any messages can have a sender, but
it is not required. (Should it be?)

Message Types from client to core:

|00|00|00|01| - Register message, contains register string
|00|00|00|03| - Subscribe Message
JSON payload should have the message name to subscribe to
|00|00|00|05| - Unsubscribe
|00|00|00|07| - Publish message
|00|00|00|0B| - Point-To-Point directed message
|00|00|00|0D| -
|00|00|00|0F| -

All messaages from server->client have a timestamp:

|    Unix timestmap     |  Payload  |
|11|22|33|44|55|66|77|88|B1|B2|B3|BN|

Message types from server to client
|00|00|00|02| - Auth reply, JSON reply giving server information
|00|00|00|04| - Subscribe reply, contains list of providers
|00|00|00|06| - UnSubscribe reply
|00|00|02|04| - Someone subscribed message
|00|00|02|08| - Publish message


On Client, to notify the core that you have a message to advertize:

DynaMsg::advertizeMessage(QString message)
-calls: generateAdvertizeRequest(QString) : QByteArray
--Adds message structure message type to the front of the message
-calls: generateCorePacket(QByteArray) : QByteArray
--Wraps header, length, checksum, etc
-calls: socket->write(QByteArray)
--Sends message out the socket

On Client, subscribe to a recieve a message whenever a new one is published:
DynaMsg::subscribeToMessage(QString message)
-calls: generateSubscribeRequest(QString) : QByteArray
--Adds message structure message type to the front of the message
-calls: generateCorePacket(QByteArray) : QByteArray
--Adds a header, length, and checksum if needed
-calls: socket->write(QByteArray)
--Sends message out the socket

On client, recieve thread:
DynaMsg::onDataReady()
-Adds incoming bytes to buffer
-calls: parsePackets(QByteArray) on the buffer
--Scans through the buffer for any packets
--Parses header, length, and grabs single packet
--calls: parseSingle(QByteArray)
---checks message type, fires appropriate event.


On Server:
DynaMsg::readyRead()
Adds bytes to a per-client buffer
calls parseBuffer(QTcpSocket*)

DynaMsg::parseBuffer(QTcpSocket*)
Figures out if there is a whole message in the buffer,
if so, take it out of the buffer and add it to the processing queue.
Trigger the next queue hop.

DynaMsg::processQueue(QTcpSocket*)
Process message queue.
If it's a subscribe message, add to subscriber queue
If it's a advertize message, add to the advertize queue
etc


Binary blob multi part. First 4 bytes are index, second 4 are total, then the rest are the blob.
Eg:
|Packet Num |Total packets|Binary bytes|
|L1|L2|L3|L4|T1|T2|T3| T4 |B1|B2|BN-8|


One core running per machine

Multiple cores per net

Client connects to core, authenticates, sends subscribe, advertize, and broadcast requests
Cores tell each other which clients are connected, which advertizes, and which subscribers needed
Cores can also transmit broadcast requests depending on the netmask

Core networks set up like dbus

A media library machine would have a core with the name:
/local/media/library
Then some clients, with the names:
/local/media/library/media_scanner
/local/media/library/media_metadata
/local/media/library/media_internet_info

When you broadcast out to /local/media, then everyone on /local/media would get it
You could broadcast out to /local, or /, and then everyone on local, or globally, would get it.



*/


#include <QObject>
#include <QMap>
#include "dynamsgparser.h"
#include "dynamsgdatastore.h"
#include "dynamsgconnection.h"
#include <dynamsgserver.h>

class QTcpSocket;
class QTcpServer;

class DynaMsg : public QObject
{
	Q_OBJECT
public:
	explicit DynaMsg(QString key, QObject *parent = 0);
	void setConnectionPass(QString pass);
	void connectToHost(QString address, int portNum);
	void connectToCore(QString address, int portNum);
	void disconnectFromCore();
	void sendJsonMessage(QString target, QJsonObject object);

	void sendMessage(QString target,QByteArray content,QString sender = QString());

	void sendOpenPortRequest(quint64 target,int portnumber);
	void sendConnectToPortRequest(quint64 target,QString hostaddress, int portnumber);
	void subscribeMessage(QString messageName);
	void publishMessage(QString messageName,QByteArray content);
	void sendSubscribedMessage(quint64 target,QString messageName,QByteArray content);
	void sendOpenPortRequest(quint64 target,quint64 sender,quint16 port);
	QByteArray generateCorePacket(QByteArray messageBytes);
	void setName(QString key);
	const QString & name() { return m_key; }
	void startListen(int port);
private:
	DynaMsgDataStore *m_ipcDataStore;
	QByteArray generateSubscribeMessage(QString messageName);
	QByteArray generatePublishMessage(QString messageName,QByteArray payload);
	QByteArray generateSendMessage(QString target,QString sender,QByteArray payload);
	QString m_key;
	QString m_pass;


	//These sockets have connected and authenticated.
//	QMap<QString,DynaMsgConnection*> m_serverSocketMap;
//	QMap<QTcpSocket*,QString> m_socketServerNameMap;
	QMap<QString,DynaMsgConnection*> m_authedConnectionMap;

	DynaMsgConnection* m_coreConnection;
	DynaMsgServer *m_server;
	QList<DynaMsgConnection*> m_unAuthedConnectionList;
	QList<DynaMsgConnection*> m_authedConnectionList;
	QList<quint64> m_connectionIdList;
	QMap<quint64,DynaMsgConnection*> m_connectionIdToConnectionMap;
//	QMap<quint64,DynaMsgConnection*> m_authedConnectionMap;
	//These sockets have connected, but not authenticated.
//	QByteArray m_socketBuffer;
//	QList<QTcpSocket*> m_serverSocketListPreAuth;
//	DynaMsgParser *m_parser;
//	QMap<QTcpSocket*,QByteArray> m_serverSocketBuffer;
	QByteArray makeJsonPacket(QJsonObject message);
	void checkBuffer();
	QByteArray generateAuthMessage(QString key);

	QList<QByteArray> m_packetBuffer;
	quint64 getRandomId();
signals:
	void si_incomingMessage(QByteArray message);
	void si_incomingSubscribedMessage(QString id, QByteArray message);
	void si_connected(); // Emitted when we're connected
	void coreConnected(); //Emitted once we've connected and authenticated to the core.
	void coreRejected(); // Emitted if the core rejects our authentication attempt.
	void clientConnected(); //Emitted once a client authenticates.
	void si_disconnected();
	void si_jsonPacketReceived(QJsonObject message);
	void si_subscribeMessage(QString message);
	void si_publishMessage(QString name, QByteArray payload);
	void si_ptpMessageReceived(quint64 targetid,quint64 senderid, QString target,QByteArray payload);
	void si_ptpMessageReceived(QByteArray payload);
	void subscribeRequest(quint64 target, quint64 sender,QString name);
	void incomingSubscribedMessage(QString name,QByteArray payload);
	void incomingPublishMessage(quint64 sender,QString name,QByteArray payload);

public slots:
private slots:
	void remoteDisconnected();
	void coreConnectionConnected();
	void newClientConnected();
	void authResponse(quint64 target, quint64 sender, QJsonObject caps);
	void authRequest(quint64 target, quint64 sender, QJsonObject authobject);
	void incomingPortOpenRequest(quint64 requester);
	void ptpMessageReceived(quint64 targetid, quint64 senderid, QString target, QByteArray payload);
//	void subscribeRequest(quint64 target, quint64 sender,QString name);

};

#endif // DynaMsg_H
