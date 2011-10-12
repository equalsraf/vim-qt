#ifndef __VIM_QT_FONTDIALOG__
#define __VIM_QT_FONTDIALOG__

#include <Qt/QtGui>

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
	QListWidget *fontList;
	QListWidget *sizeList;
	QDialogButtonBox *buttons;
	QLineEdit *preview;
	QFontDatabase fontDatabase;
};

#endif
