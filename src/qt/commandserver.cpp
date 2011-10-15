#include "commandserver.moc"
#include "vimwrapper.h"

extern "C" {
#include "vim.h"
}

CommandServer* CommandServer::instance = new CommandServer();

CommandClient::CommandClient(QLocalSocket *sock, QObject *parent)
:QObject(parent), m_socket(sock)
{
	connect(m_socket, SIGNAL(disconnected()),
		m_socket, SLOT(deleteLater()));

	stream = new QDataStream(m_socket);
	stream->setVersion(QDataStream::Qt_4_0);

	connect(m_socket, SIGNAL(readyRead()),
			this, SLOT(readRequest()));
}

void CommandClient::readRequest()
{
	bool asExpr;
	QString cmd;

	while ( m_socket->bytesAvailable() ) {
		*stream >> cmd >> asExpr;
		if ( cmd.isEmpty() ) {
			continue;
		}

		if ( asExpr ) {
			char_u *res = eval_client_expr_to_string( (char_u *)VimWrapper::convertTo(cmd).constData() );
			*stream << VimWrapper::convertFrom((char*)res);
		} else {
			// Do we need to poke the loop ?
			server_to_input_buf((char_u *)VimWrapper::convertTo(cmd).constData());
			*stream << "";
		}
	}
}

CommandServer::CommandServer(QObject *parent)
:QLocalServer(parent)
{
	connect(this, SIGNAL(newConnection()),
			this, SLOT(handleRequest()));
}

CommandServer* CommandServer::getInstance()
{
	return instance;
}

void CommandServer::setBaseName(const QString& name)
{
	m_name = name;
}

bool CommandServer::listen()
{
	if ( !QLocalServer::listen(m_name) ) {
		QString trySocketName;
		int idx = 1;
		do {
			// FIXME: Fallback strategy using QUuid
			trySocketName = m_name + QString::number(idx++);
			if ( QFileInfo(trySocketName).exists() ) {
				continue;
			}
		} while ( !QLocalServer::listen(trySocketName) );
		socketName =  trySocketName;
	}
	
	return true;
}

void CommandServer::handleRequest()
{
	QLocalSocket *clientConnection = nextPendingConnection();
	if ( !clientConnection ) {
		return;
	}
	CommandClient *cli = new CommandClient(clientConnection, this);


}
