/*
 * if_qtcmdsrv.c: Functions for passing commands through via Qt IPC.
 *
 * This serializes Qt types over a set of local sockets 
 *
 * The API we use here is mostly compatible with the WIN32 API however the
 * protocol is not the same, @see os_mswin.c.
 *
 * The stream protocol is as follows:
 * ClientQuery:		| cmd(QString | expr(bool)) |
 * ServerResponse:	| response(QString) |
 *
 */
#include <Qt/QtCore>
#include <Qt/QtNetwork>
#include "commandserver.h"
#include "vimwrapper.h"

extern "C" {

#include "vim.h"

static CommandServer *serverSocket;
static QMap<int, QLocalSocket*> serverConnections;


// A quick hack to map servernames into HWND
static QStringList vimServers;

static QString socketFolder() 
{
	char *env = getenv("USER");
	if (!env) {
		env = getenv("USERNAME");
	}

	if ( !env ) {
		return "";
	}

	QString folderName = QString("vim-qt-%1").arg(env);
	QDir tmp = QDir::temp();
	tmp.mkpath(folderName);
	return tmp.filePath(folderName);
}

/*
 * @return a newline separated list in allocated memory or NULL.
 *
 * FIXME: not sure how portable this is
 */
char_u *
serverGetVimNames(void)
{
	QDir dir(socketFolder());
	dir.setFilter(QDir::System | QDir::NoDotAndDotDot );

	if ( !dir.exists() ) {
		return NULL;
	}
	qDebug() << __func__;
	QStringList files = dir.entryList();
	QString socketList = files.join("\n");

	QByteArray data = socketList.toAscii();
	char_u *buffer = (char_u *)calloc( data.length(), sizeof(char_u));
	if ( buffer == NULL ) {
		return NULL;
	}

	for (int i=0; i<data.length(); i++ ) {
		buffer[i] = (char_u)data.constData()[i];
	}

	return buffer;
}


/*
 * Initialise the message handling process.
 *
 */
void
serverInitMessaging(void)
{
	// Nothing to do
	qDebug() << __func__;
	serverSocket = CommandServer::getInstance();
}

//
// Server functions
//


/**
 *
 * Set server name
 *
 * In practice this initialises the server. In practice
 * the server implementation does not live here @see commandserver.h
 *
 * @param Server name
 */
void
serverSetName(char_u *name)
{
	QDir dir(socketFolder());
	QFileInfo fi(dir.filePath((char*)name));
	QString socketName = fi.absoluteFilePath();

	serverSocket->setBaseName(socketName);
}

/**
 * Send a reply string (notification) to client with id "name".
 *
 * @eturn -1 if the window is invalid.
 *
 * @param name Where to send
 * @param reply What to send
 */
int
serverSendReply(char_u *name, char_u *reply) 
{
	qDebug() << __func__ << (char*) name << (char*) reply;
	return -1;
}


//
// Client commands
//

/*
 * Get a reply from server
 *
 * @param remove Consume the message, the caller must free it then. //FIXME: Not-implemented
 * @param server The id of a server connection, @see serverSendToVim
 * @param wait Block until a message arrives (or the server exits).
 * @param expr_res Return code, 0 on success and -1 on error
 */
char_u *
serverGetReply(HWND server, int *expr_res, int remove, int wait)
{
	if ( expr_res ) {
		*expr_res = -1;
	}

	QString reply;
	QLocalSocket *sock = serverConnections.value(server);
	if ( sock == NULL ) {
		return NULL;
	}

	QDataStream stream(sock);
	stream.setVersion(QDataStream::Qt_4_0);

	// Get reply - wait if necessary
	if ( wait && !sock->waitForReadyRead() ) {
		sock->close();
		return NULL;
	}
	if ( sock->bytesAvailable() ) {
		stream >> reply;
	} else {
		sock->close();
		return NULL;
	}

	if ( expr_res ) {
		*expr_res = 0;
	}

	if ( !reply.isEmpty() ) {
		return VimWrapper::copy(VimWrapper::convertTo(reply));
	}
	return NULL;
}

/**
 *
 * Send command to VIM server
 *
 * @param name Where to send
 * @param cmd What to send
 * @param result Result of eval'ed expression
 * @param ptarget HWND of server
 * @param asExpr Expression or keys?
 * @param silent don't complain about no server
 *
 * @return 0 on success
 */
int
serverSendToVim(char_u *name, char_u *cmd, char **result, void *ptarget, int asExpr, int silent)
{
	QDir dir( socketFolder() );
	QString remotecmd = VimWrapper::convertFrom(cmd);

	QLocalSocket *sock = new QLocalSocket();
	QObject::connect(sock, SIGNAL(disconnected()), sock, SLOT(deleteLater()));
	sock->connectToServer(dir.filePath((char*)name));
	if ( !sock->waitForConnected(3) ) {
		return -1;
	}

	QDataStream stream(sock);
	stream.setVersion(QDataStream::Qt_4_0);

	//
	// Be carefull, the (bool) cast is absolutely necessary
	// otherwise the stream will always read as false.
	stream << remotecmd << (bool)asExpr;
	if ( !sock->waitForBytesWritten() ) {
		sock->close();
		return -1;
	}

	quintptr fd = sock->socketDescriptor();
	serverConnections.insert(fd, sock);
	if (ptarget) {
		*(HWND*)ptarget = fd;
	}

	// Get reply
	if ( !sock->waitForReadyRead() ) {
		sock->close();
		return -1;
	}

	QString exp_result;
	stream >> exp_result;

	if ( asExpr != TRUE ) {
		return 0;
	}

	*result = (char*)VimWrapper::copy(VimWrapper::convertTo(exp_result));
	return 0;
}

} // extern "C"
