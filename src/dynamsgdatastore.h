#ifndef DynaMsgDATASTORE_H
#define DynaMsgDATASTORE_H

#include <QString>
#include <QByteArray>
#include <QMap>
#include <QList>
class DynaMsgDataStore
{
public:
	DynaMsgDataStore();
	void addMessage(QString messagename,QByteArray messagepayload);
private:
	QMap<QString,QList<QByteArray> > m_messageMap;
};

#endif // DynaMsgDATASTORE_H
