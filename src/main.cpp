#include <QCoreApplication>
#include "dynatest.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	DynaTest dt;
	dt.start();
	return a.exec();
}

