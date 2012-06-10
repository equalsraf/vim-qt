#ifndef __VIM_QT_FONTDIALOG__
#define __VIM_QT_FONTDIALOG__

#include <QtGui>

class FontDialog: public QDialog
{
	Q_OBJECT
public:
	FontDialog(QWidget *parent=0);
	QFont selectedFont();

	static QFont getFont(bool *ok, const QFont &, QWidget *parent=0);
	void selectCurrentFont(const QFont&);

protected slots:
	void updateFonts();
	void fontSelected();

private:
	// layouts and widgets
	QHBoxLayout   *hlayout;
	QVBoxLayout   *vlayout;
	QVBoxLayout   *vlayoutInfo;
	QListWidget   *fontList;
	QListWidget   *sizeList;
	QDialogButtonBox *buttons;
	QLineEdit     *preview;
	QScrollArea   *scrollInfo;
	QGroupBox     *groupboxInfo;
	QLabel        *styleInfo;
	QLabel        *writingInfo;
	QLabel        *separatorInfo;
	// data
	QFontDatabase fontDatabase;
	QString       regularStyle;
	// constants
	static const  QRegExp regular_rx;
};

#endif
