#include "fontdialog.moc"

/**
 * Custom font dialog
 *
 * This custom font selection dialog allows us to do
 * things the regular one does not:
 * - Enforce use of fixed Pitch fonts
 * - Enforce font style to "Normal"
 *
 * -----------------------------------
 * |                |                |
 * |   Family       |     Font       |
 * |    List        |     Sizes      |
 * |---------------------------------|
 * |          Preview font           |
 * |_________________________________|
 * |                Buttons          |
 * |_________________________________|
 *
 * FIXME: The dialog could use some visual improvements,
 *
 */

FontDialog::FontDialog(QWidget *parent)
:QDialog(parent)
{
	QHBoxLayout *hlayout = new QHBoxLayout();
	QVBoxLayout *vlayout = new QVBoxLayout(this);

	sizeList = new QListWidget(this);
	fontList = new QListWidget(this);

	// Populate fonts list
	foreach (const QString &family, fontDatabase.families()) {
		if ( !fontDatabase.isFixedPitch(family) ) {
			continue;
		}

		QListWidgetItem *item = new QListWidgetItem(fontList);
		item->setText(family);
	}
	
	connect(fontList, SIGNAL(itemSelectionChanged()),
			this, SLOT(updateFonts()));

	connect(sizeList, SIGNAL(itemSelectionChanged()),
			this, SLOT(fontSelected()));


	// Dialog buttons
	buttons = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	buttons->button(QDialogButtonBox::Ok)->setEnabled(false);

	connect( buttons->button(QDialogButtonBox::Ok), SIGNAL(clicked()), 
			this, SLOT(accept()));
	connect( buttons->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), 
			this, SLOT(reject()));

	// preview
	preview = new QLineEdit(this);
	preview->setText("The quick brown fox jumps over the lazy dog");
	preview->setAlignment(Qt::AlignCenter);
	preview->setEnabled(false);

	hlayout->addWidget(fontList);
	hlayout->addWidget(sizeList);
	vlayout->addLayout(hlayout);
	vlayout->addWidget(preview);
	vlayout->addWidget(buttons);

	this->setLayout(vlayout);
}


void FontDialog::updateFonts()
{
	sizeList->clear();
	QStringList sizes;

	QListWidgetItem *current = fontList->currentItem();
	if ( current == NULL ) {
		return;
	}

	QList<int> fsizes = fontDatabase.smoothSizes(current->text(), "Normal");
	if ( fsizes.isEmpty() ) {
		fsizes = fontDatabase.pointSizes(current->text(), "Normal");
	}

	foreach ( int points, fsizes ) {
		QListWidgetItem *item = new QListWidgetItem(sizeList);
		item->setText(QString::number(points));
	}

	buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
	preview->setEnabled(false);
}

void FontDialog::fontSelected()
{
	QListWidgetItem *currentF = fontList->currentItem();
	QListWidgetItem *currentS = sizeList->currentItem();

	if ( currentF == NULL || currentS == NULL ) {
		return;
	}

	buttons->button(QDialogButtonBox::Ok)->setEnabled(true);

	QFont f = fontDatabase.font( currentF->text(),
			"Normal", currentS->text().toInt());
	preview->setFont(f);
	preview->setEnabled(true);
}

QFont FontDialog::selectedFont()
{
	if ( fontList->currentItem() && sizeList->currentItem() ) {
		return QFont( fontList->currentItem()->text(), sizeList->currentItem()->text().toInt() );
	}

	return QFont();
}

QFont FontDialog::getFont(bool *ok, const QFont& oldfont, QWidget *parent)
{
	FontDialog dialog;
	dialog.selectCurrentFont(oldfont);

	if ( dialog.exec() == QDialog::Accepted ) {
		*ok = true;
		return dialog.selectedFont();
	}

	*ok = false;
	return QFont();
}

void FontDialog::selectCurrentFont(const QFont& f)
{
	QList<QListWidgetItem *> items = fontList->findItems(f.family(), Qt::MatchFixedString);
	if ( items.size() > 0 ) {
		fontList->setCurrentItem(items.at(0));

		QList<QListWidgetItem *> sizeItems = sizeList->findItems(QString::number(f.pointSize()), Qt::MatchFixedString);
		if ( sizeItems.size() > 0 ) {
			sizeList->setCurrentItem(sizeItems.at(0));
		}
	}
}


