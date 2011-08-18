/*
 * if_qtcmdsrv.c: Functions for passing commands through via Qt IPC.
 *
 * This serializes Qt types over a set of local sockets 
 *
 * The API we use here is mostly compatible with the WIN32 API however the
 * protocol is not the same.
 * @see os_mswin.c
 *
 *
 *
 */
#include <Qt/QtCore>
#include <Qt/QtNetwork>
#include "commandserver.h"
#include "vimwrapper.h"

extern "C" {

#include "vim.h"

static CommandServer serverSocket;

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
	qDebug() <<__func__;

	QDir dir(socketFolder());
	dir.setFilter(QDir::Files | QDir::NoDotAndDotDot );

	if ( !dir.exists() ) {
		return NULL;
	}
	
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



/**
 * Send a reply string (notification) to client with id "name".
 * Return -1 if the window is invalid.
 *
 * @param name Where to send
 * @param reply What to send
 */
int
serverSendReply(char_u *name, char_u *reply) 
{
	qDebug() <<__func__ << (char*)name << (char*)reply;
	return -1;
}

/*
 * Get a reply from server "server".
 * When "expr_res" is non NULL, get the result of an expression, otherwise a
 * server2client() message.
 * When non NULL, point to return code. 0 => OK, -1 => ERROR
 * If "remove" is TRUE, consume the message, the caller must free it then.
 * if "wait" is TRUE block until a message arrives (or the server exits).
 */
char_u *
serverGetReply(HWND server, int *expr_res, int remove, int wait)
{
	qDebug() <<__func__ << server;
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
	QString remotecmd = VimWrapper::convertFrom((char *)cmd);

	QLocalSocket sock;
	sock.connectToServer(dir.filePath((char*)name));
	if ( !sock.waitForConnected(3) ) {
		return -1;
	}

	QDataStream stream(&sock);
	stream.setVersion(QDataStream::Qt_4_0);

	//
	// Be carefull, the (bool) cast is absolutely necessary
	// otherwise the stream alway read as false.
	stream << remotecmd << (bool)asExpr;
	if ( !sock.waitForBytesWritten() ) {
		sock.close();
		qDebug() << "write";
		return -1;
	}

	if ( asExpr != TRUE ) {
		return 0;
	}

	// Get reply
	if ( !sock.waitForReadyRead() ) {
		qDebug() << "read" << sock.error();

		sock.close();
		return -1;
	}

	QString exp_result;
	stream >> exp_result;
	qDebug() << exp_result;
	*result = (char*)VimWrapper::copy(VimWrapper::convertTo(exp_result));

	sock.close();
	return 0;
}

void
serverSetName(char_u *name)
{
	qDebug() << __func__ << (char*)name;

	QDir dir(socketFolder());
	QFileInfo fi(dir.filePath((char*)name));
	QString socketName = fi.absoluteFilePath();

	if ( fi.exists() ) {
		QUuid uuid = QUuid::createUuid();
		socketName = QString((char*)name) + uuid.toString();
	}
	QByteArray data = socketName.toAscii();

	char_u *buffer = alloc(data.length());
	for (int i=0; i< data.length(); i++) {
		buffer[i] = data.constData()[i];
	}
	serverName = buffer;

	serverSocket.listen(socketName);

#ifdef FEAT_EVAL
	/* Set the servername variable */
	set_vim_var_string(VV_SEND_SERVER, serverName, -1);
#endif
}

/*
 * Initialise the message handling process.
 *
 */
void
serverInitMessaging(void)
{
	// Nothing to do	
}


} // extern "C"
