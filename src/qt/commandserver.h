#ifndef __VIM_QT_COMMANDSERVER__
#define __VIM_QT_COMMANDSERVER__

#include <Qt/QtNetwork>

class CommandClient: public QObject
{
	Q_OBJECT
public:
	CommandClient(QLocalSocket *, QObject *parent=0);

protected slots:
	void readRequest();
private:
	QLocalSocket *m_socket;
	QDataStream *stream;
};

class CommandServer: public QLocalServer
{
	Q_OBJECT
public:
	CommandServer(QObject *parent=0);

protected slots:
	void handleRequest();

};

#endif
