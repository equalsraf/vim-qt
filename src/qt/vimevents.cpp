#include "vimevents.h"
#include "vimwrapper.h"

VimEvent::VimEvent(VimWrapper &vim)
:vim(vim)
{
}

ResizeEvent::ResizeEvent(VimWrapper &vim, int w, int h)
:VimEvent(vim), width(w), height(h)
{
}

void ResizeEvent::handle()
{
	vim.guiResizeShell(width, height);
}

CloseEvent::CloseEvent(VimWrapper& vim)
:VimEvent(vim)
{
}

void CloseEvent::handle()
{
	vim.guiShellClosed();
}

DropEvent::DropEvent(VimWrapper& vim, const QPoint& pos, unsigned int mod, QList<QUrl> urls)
:VimEvent(vim), pos(pos), mod(mod), urls(urls)
{
}

void DropEvent::handle()
{
	vim.guiHandleDrop(pos, mod, urls);
}

