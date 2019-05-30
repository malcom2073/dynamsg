#include "dynatest.h"
#include <QDebug>
#include <QCoreApplication>
#include <QTimer>
DynaTest::DynaTest(QObject *parent) : QThread(parent)
{


}
void DynaTest::run()
{
	bool allgood = true;
	QStringList testresults;
	qDebug() << "Starting tests...";
	if (runValidAuthConnectTest()) testresults += "***SUCCEED*** : runValidAuthConnectTest";
	else
	{
		testresults += "***FAILURE*** : runValidAuthConnectTest";
		allgood = false;
	}
	if (runInValidAuthConnectTest()) testresults+= "***SUCCEED*** : runInValidAuthConnectTest";
	else
	{
		testresults+= "***FAILURE*** : runInValidAuthConnectTest";
		allgood = false;
	}
	if (runNullAuthConnectTest()) testresults+="***SUCCEED*** : runNullAuthConnectTest";
	else
	{
		testresults+= "***FAILURE*** : runNullAuthConnectTest";
		allgood = false;
	}
	if (runSendMessageTest()) testresults+="***SUCCEED*** : runSendMessageTest";
	else
	{
		testresults+= "***FAILURE*** : runSendMessageTest";
		allgood = false;
	}
	if (runReceiveMessageTest()) testresults+= "***SUCCEED*** : runReceiveMessageTest";
	else
	{
		testresults+= "***FAILURE*** : runReceiveMessageTest";
		allgood = false;
	}
	if (clientCoreClientMessageTest()) testresults+= "***SUCCEED*** : clientCoreClientMessageTest";
	else
	{
		testresults+="***FAILURE*** : clientCoreClientMessageTest";
		allgood = false;
	}
	if (clientCoreClientDisconnectTest()) testresults+= "***SUCCEED*** : clientCoreClientDisconnectTest";
	else
	{
		testresults+="***FAILURE*** : clientCoreClientDisconnectTest";
		allgood = false;
	}



	if (allgood)
	{
		foreach (QString result,testresults)
		{
			qDebug() << result;
		}
		qDebug() << "All tests ran, all test succeeded";
		QCoreApplication::instance()->exit(0);
	}
	else
	{
		foreach (QString result,testresults)
		{
			qDebug() << result;
		}
		qDebug() << "All tests ran, at least one test FAILED";
		QCoreApplication::instance()->exit(-1);
	}
}
bool DynaTest::runValidAuthConnectTest()
{
	testcore = new DynaMsg("core");
	bool clientconnect = false;
	bool coreconnect = false;
	bool spinloop = true;
	connect(testcore,&DynaMsg::clientConnected,[this,&coreconnect]() {
		coreconnect = true;
	});

	testcore->startListen(2073);
	DynaMsg *c1 = new DynaMsg("c1");
	c1->setConnectionPass("pass");
	c1->connectToCore("localhost",2073);
	connect(c1,&DynaMsg::coreConnected,[this,c1,&clientconnect,&spinloop]() {
		qDebug() << "**TEST** Client says core connected";
		clientconnect = true;
		spinloop = false;
	});
	QTimer::singleShot(2000,[this,c1,&spinloop]() {
		spinloop = false;
	});
	while (spinloop)
	{
		QCoreApplication::processEvents();
		QThread::msleep(1);
	}
	c1->disconnectFromCore();
	delete c1;
	delete this->testcore;
	this->testcore = 0;
	return clientconnect && coreconnect;
}
bool DynaTest::runInValidAuthConnectTest()
{
	testcore = new DynaMsg("core");
	bool clientconnect = false;
	bool coreconnect = true;
	bool spinloop = true;
	connect(testcore,&DynaMsg::clientConnected,[this,&coreconnect]() {
		//coreconnect = true;
	});

	testcore->startListen(2073);
	DynaMsg *c1 = new DynaMsg("c1");
	c1->setConnectionPass("asdf");

	c1->connectToCore("localhost",2073);
	connect(c1,&DynaMsg::coreConnected,[this,c1,&clientconnect,&spinloop]() {
		qDebug() << "**TEST** Client says core connected";
		clientconnect = false;
		spinloop = false;
	});
	connect(c1,&DynaMsg::coreRejected,[this,c1,&clientconnect,&spinloop]() {
		qDebug() << "**TEST** Client says core rejected conection";
		clientconnect = true;
		spinloop = false;
	});
	QTimer t;
	t.setSingleShot(true);
	t.setInterval(3000);
	connect(&t,&QTimer::timeout,[this,c1,&spinloop]() {
		spinloop = false;
	});
	t.start();
	while (spinloop)
	{
		QCoreApplication::processEvents();
		QThread::msleep(1);
	}
	t.stop();
	c1->disconnectFromCore();
	delete c1;
	delete this->testcore;
	this->testcore = 0;
	return clientconnect && coreconnect;
}

bool DynaTest::runNullAuthConnectTest()
{
	testcore = new DynaMsg("core");
	bool clientconnect = false;
	bool coreconnect = false;
	bool spinloop = true;
	connect(testcore,&DynaMsg::clientConnected,[this,&coreconnect]() {
		coreconnect = true;
	});

	testcore->startListen(2073);
	DynaMsg *c1 = new DynaMsg("c1");
	c1->connectToCore("localhost",2073);
	connect(c1,&DynaMsg::coreConnected,[this,c1,&clientconnect,&spinloop]() {
		qDebug() << "**TEST** Client says core connected";
		clientconnect = true;
		spinloop = false;
	});
	QTimer t;
	t.setSingleShot(true);
	t.setInterval(3000);
	connect(&t,&QTimer::timeout,[this,c1,&spinloop]() {
		spinloop = false;
	});
	t.start();

	while (spinloop)
	{
		QCoreApplication::processEvents();
		QThread::msleep(1);
	}
	t.stop();
	c1->disconnectFromCore();
	delete c1;
	delete this->testcore;
	this->testcore = 0;
	return clientconnect && coreconnect;
}
bool DynaTest::runSendMessageTest()
{
	testcore = new DynaMsg("core");
	bool clientconnect = false;
	bool coreconnect = false;
	bool coremsg = false;
	bool spinloop = true;
	connect(testcore,&DynaMsg::clientConnected,[this,&coreconnect]() {
		coreconnect = true;
	});
	connect(testcore,&DynaMsg::si_incomingMessage,[this,&coremsg,&spinloop](QByteArray msg) {
		qDebug() << msg;
		spinloop = false;
		coremsg = true;
	});

	testcore->startListen(2073);
	DynaMsg *c1 = new DynaMsg("c1");
	c1->setConnectionPass("pass");
	c1->connectToCore("localhost",2073);
	connect(c1,&DynaMsg::coreConnected,[this,c1,&clientconnect]() {
		qDebug() << "**TEST** Client says core connected";
		clientconnect = true;
		c1->sendMessage("core",QString("WEEEEE").toLatin1());
	});
	QTimer t;
	t.setSingleShot(true);
	t.setInterval(3000);
	connect(&t,&QTimer::timeout,[this,&spinloop]() {
		spinloop = false;
	});
	t.start();
	while (spinloop)
	{
		QCoreApplication::processEvents();
		QThread::msleep(1);
	}
	t.stop();
	c1->disconnectFromCore();
	delete c1;
	delete testcore;
	testcore = 0;
	return clientconnect && coreconnect && coremsg;
}

bool DynaTest::runReceiveMessageTest()
{
	testcore = new DynaMsg("core");
	bool clientconnect = false;
	bool coreconnect = false;
	bool coremsg = false;
	bool spinloop = true;
	connect(testcore,&DynaMsg::clientConnected,[this,&coreconnect]() {
		coreconnect = true;
	});

	testcore->startListen(2073);
	DynaMsg *c1 = new DynaMsg("c1");
	c1->setConnectionPass("pass");
	c1->connectToCore("localhost",2073);
	connect(c1,&DynaMsg::coreConnected,[this,c1,&clientconnect]() {
		qDebug() << "**TEST** Client says core connected";
		clientconnect = true;
		testcore->sendMessage("c1",QString("WEE123").toLatin1());
//		c1->sendMessage("core",QString("WEEEEE").toLatin1());
	});
	connect(c1,&DynaMsg::si_incomingMessage,[this,&coremsg,&spinloop](QByteArray msg) {
		qDebug() << msg;
		spinloop = false;
		coremsg = true;
	});
	QTimer t;
	t.setSingleShot(true);
	t.setInterval(3000);
	connect(&t,&QTimer::timeout,[this,&spinloop]() {
		spinloop = false;
	});
	t.start();
	while (spinloop)
	{
		QCoreApplication::processEvents();
		QThread::msleep(1);
	}
	t.stop();
	c1->disconnectFromCore();
	delete c1;
	delete testcore;
	testcore = 0;
	return clientconnect && coreconnect && coremsg;
}



bool DynaTest::clientCoreClientMessageTest()
{
	//This tests two clients and one core, where
	//one client sends a PTP message to the other, through the core,
	//using the client name as a target.
	testcore = new DynaMsg("core");
	bool c1connect = false;
	bool c2connect = false;
	bool coreconnect = false;
	bool c2recvmessage = false;
	bool spinloop = true;
	connect(testcore,&DynaMsg::clientConnected,[this,&coreconnect]() {
		coreconnect = true;
	});

	testcore->startListen(2073);
	DynaMsg *c1 = new DynaMsg("c1");
	DynaMsg *c2 = new DynaMsg("c2");
	c1->setConnectionPass("pass");
	c1->connectToCore("localhost",2073);
	connect(c1,&DynaMsg::coreConnected,[this,c1,c2,&c1connect]() {
		qDebug() << "**TEST** Client says core connected";
		c1connect = true;
		c2->setConnectionPass("pass");
		c2->connectToCore("localhost",2073);
//		c1->sendMessage("core",QString("WEEEEE").toLatin1());
	});
	connect(c2,&DynaMsg::coreConnected,[this,c1,c2,&c2connect]() {
		qDebug() << "**TEST** Client says core connected";
		c2connect = true;
//		testcore->sendMessage("c1",QString("WEE123").toLatin1());
		c1->sendMessage("c2",QString("WEEEEE").toLatin1());
	});
	connect(c2,&DynaMsg::si_incomingMessage,[this,&c2recvmessage,&spinloop](QByteArray msg) {
		qDebug() << msg;
		spinloop = false;
		c2recvmessage = true;
	});
	QTimer t;
	t.setSingleShot(true);
	t.setInterval(3000);
	connect(&t,&QTimer::timeout,[this,c1,c2,&spinloop]() {
		spinloop = false;
	});
	t.start();
	while (spinloop)
	{
		QCoreApplication::processEvents();
		QThread::msleep(1);
	}
	t.stop();
	delete c1;
	delete c2;
	delete testcore;
	testcore = 0;
	return c1connect && c2connect && coreconnect && c2recvmessage;
}
bool DynaTest::clientCoreClientDisconnectTest()
{
	//Tests same as clientCoreClient, but disconnects before the message is sent to ensure
	// the core acts appropriatly.
	//This tests two clients and one core, where
	//one client sends a PTP message to the other, through the core,
	//using the client name as a target.
	testcore = new DynaMsg("core");
	bool c1connect = false;
	bool c2connect = false;
	bool coreconnect = false;
	bool c2recvmessage = false;
	bool spinloop = true;
	connect(testcore,&DynaMsg::clientConnected,[this,&coreconnect]() {
		coreconnect = true;
	});

	testcore->startListen(2073);
	DynaMsg *c1 = new DynaMsg("c1");
	DynaMsg *c2 = new DynaMsg("c2");
	c1->setConnectionPass("pass");
	c1->connectToCore("localhost",2073);
	//Once C1 connects, tell C2 to connect.
	connect(c1,&DynaMsg::coreConnected,[this,c1,c2,&c1connect]() {
		qDebug() << "**TEST** Client says core connected";
		c1connect = true;
		c2->setConnectionPass("pass");
		c2->connectToCore("localhost",2073);
//		c1->sendMessage("core",QString("WEEEEE").toLatin1());
	});

	//Once C2 is connected, disconnect C1 and have C2 send a message to C1.
	connect(c2,&DynaMsg::coreConnected,[this,c1,c2,&c2connect]() {
		qDebug() << "**TEST** Client says core connected";
		c2connect = true;
//		testcore->sendMessage("c1",QString("WEE123").toLatin1());
		c1->disconnectFromCore();
		c2->sendMessage("c1",QString("WEEEEE").toLatin1());
	});

	//We should get some sort of notification here...
	connect(c1,&DynaMsg::si_incomingMessage,[this,&c2recvmessage,&spinloop](QByteArray msg) {
		qDebug() << msg;
		spinloop = false;
		c2recvmessage = true;
	});
	QTimer t;
	t.setSingleShot(true);
	t.setInterval(3000);
	connect(&t,&QTimer::timeout,[this,c1,c2,&spinloop]() {
		spinloop = false;
	});
	t.start();
	while (spinloop)
	{
		QCoreApplication::processEvents();
		QThread::msleep(1);
	}
	t.stop();
	delete c1;
	delete c2;
	delete testcore;
	testcore = 0;
	return c1connect && c2connect && coreconnect && c2recvmessage;
}
