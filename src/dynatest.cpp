#include "dynatest.h"
#include <QDebug>
#include <QCoreApplication>
#include <QTimer>
DynaTest::DynaTest(QObject *parent) : QThread(parent)
{


}
void DynaTest::run()
{

	qDebug() << "Starting tests...";
	if (runValidAuthConnectTest()) qDebug() << "***SUCCEED*** : runValidAuthConnectTest";
	else qDebug() << "***FAILURE*** : runValidAuthConnectTest";
	if (runInValidAuthConnectTest()) qDebug() << "***SUCCEED*** : runInValidAuthConnectTest";
	else qDebug() << "***FAILURE*** : runInValidAuthConnectTest";
	if (runNullAuthConnectTest()) qDebug() << "***SUCCEED*** : runNullAuthConnectTest";
	else qDebug() << "***FAILURE*** : runNullAuthConnectTest";
	if (runSendMessageTest()) qDebug() << "***SUCCEED*** : runSendMessageTest";
	else qDebug() << "***FAILURE*** : runSendMessageTest";

}
bool DynaTest::runValidAuthConnectTest()
{
	testcore = new DynaMsg("core");
	bool clientconnect = false;
	bool coreconnect = false;
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
	});
	QTimer::singleShot(2000,[this,c1]() {
		delete c1;
		delete this->testcore;
		this->testcore = 0;
	});
	while (testcore)
	{
		QCoreApplication::processEvents();
		QThread::msleep(1);
	}
	return clientconnect && coreconnect;
}
bool DynaTest::runInValidAuthConnectTest()
{
	testcore = new DynaMsg("core");
	bool clientconnect = false;
	bool coreconnect = true;
	connect(testcore,&DynaMsg::clientConnected,[this,&coreconnect]() {
		//coreconnect = true;
	});

	testcore->startListen(2073);
	DynaMsg *c1 = new DynaMsg("c1");
	c1->setConnectionPass("asdf");

	c1->connectToCore("localhost",2073);
	connect(c1,&DynaMsg::coreConnected,[this,c1,&clientconnect]() {
		qDebug() << "**TEST** Client says core connected";
		clientconnect = false;
	});
	connect(c1,&DynaMsg::coreRejected,[this,c1,&clientconnect]() {
		qDebug() << "**TEST** Client says core rejected conection";
		clientconnect = true;
	});
	QTimer::singleShot(2000,[this,c1]() {
		delete c1;
		delete this->testcore;
		this->testcore = 0;
	});
	while (testcore)
	{
		QCoreApplication::processEvents();
		QThread::msleep(1);
	}
	return clientconnect && coreconnect;
}

bool DynaTest::runNullAuthConnectTest()
{
	testcore = new DynaMsg("core");
	bool clientconnect = false;
	bool coreconnect = false;
	connect(testcore,&DynaMsg::clientConnected,[this,&coreconnect]() {
		coreconnect = true;
	});

	testcore->startListen(2073);
	DynaMsg *c1 = new DynaMsg("c1");
	c1->connectToCore("localhost",2073);
	connect(c1,&DynaMsg::coreConnected,[this,c1,&clientconnect]() {
		qDebug() << "**TEST** Client says core connected";
		clientconnect = true;
	});
	QTimer::singleShot(2000,[this,c1]() {
		delete c1;
		delete this->testcore;
		this->testcore = 0;
	});
	while (testcore)
	{
		QCoreApplication::processEvents();
		QThread::msleep(1);
	}
	return clientconnect && coreconnect;
}
bool DynaTest::runSendMessageTest()
{
	testcore = new DynaMsg("core");
	bool clientconnect = false;
	bool coreconnect = false;
	bool coremsg = false;
	connect(testcore,&DynaMsg::clientConnected,[this,&coreconnect]() {
		coreconnect = true;
	});
	connect(testcore,&DynaMsg::si_incomingMessage,[this,&coremsg](QByteArray msg) {
		qDebug() << msg;
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
	QTimer::singleShot(2000,[this,c1]() {
		delete c1;
		delete this->testcore;
		this->testcore = 0;
	});
	while (testcore)
	{
		QCoreApplication::processEvents();
		QThread::msleep(1);
	}
	return clientconnect && coreconnect && coremsg;
}
