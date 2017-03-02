#ifndef VIM_QT_COLORTABLE
#define VIM_QT_COLORTABLE

#include <QColor>
#include <QString>
#include <QHash>

class ColorTable: public QHash<QString, QColor>
{
public:
	static QColor get(const QString& name, const QColor &fallback=QColor());

protected:
	ColorTable();

private:
	static ColorTable m_instance;
};

#endif
