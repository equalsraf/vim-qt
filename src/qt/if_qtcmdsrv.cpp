/*
 *
 * if_qtcmdsrv.c: Functions for passing commands through via Qt IPC.
 */
#include <Qt/QtCore>

extern "C" {

#include "vim.h"

/*
 * @return a newline separated list in allocated memory or NULL.
 *
 * FIXME: not sure how portable this is
 */
char_u *
serverGetVimNames(void)
{
	qDebug() <<__func__;

	char *env = getenv("USER");
	if (!env) {
		env = getenv("USERNAME");
	}

	if ( !env ) {
		return NULL;
	}

	QString foldername = QString("vim-qt-%1").arg(env);
	QDir dir = QDir::temp().filePath(foldername);
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
 * @parma name Where to send
 * @param reply What to send
 */
int
serverSendReply(char_u *name, char_u *reply) 
{
	qDebug() <<__func__ << name << reply;
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
 * @param name Where to send
 * @param cmd What to send
 * @param result Result of eval'ed expression
 * @param ptarget HWND of server
 * @param asExpr Expression or keys?
 * @param silent don't complain about no server
 */
int
serverSendToVim(char_u *name, char_u *cmd, char **result, void *ptarget, int asExpr, int silent)
{
	qDebug() << name << cmd;

	return -1;
}

void
serverSetName(char_u *name)
{
	qDebug() << __func__ << name;

}

/*
 * Initialise the message handling process.
 *
 */
void
serverInitMessaging(void)
{
	qDebug() << __func__;

}


} // extern "C"
