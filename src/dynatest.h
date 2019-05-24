#ifndef DYNATEST_H
#define DYNATEST_H

#include <QThread>
#include "dynamsg.h"

class DynaTest : public QThread
{
	Q_OBJECT
public:
	explicit DynaTest(QObject *parent = 0);
	bool runValidAuthConnectTest();
	bool runInValidAuthConnectTest();
	bool runNullAuthConnectTest();
	bool runSendMessageTest();
	bool runReceiveMessageTest();
	bool clientCoreClientMessageTest();
private:
	void run();
	DynaMsg *testcore;
signals:

public slots:
};

#endif // DYNATEST_H
