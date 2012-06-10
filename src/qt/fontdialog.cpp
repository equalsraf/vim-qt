#include "fontdialog.moc"

// some of the style names which could indicate the "regular" one
const QRegExp FontDialog::regular_rx = QRegExp("Regular|Normal|Book|Roman|Plain|Upright|Medium|Light|Sans");

/**
 * Custom font dialog
 *
 * This custom font selection dialog allows us to do
 * things the regular one does not:
 * - Enforce use of fixed-pitch fonts
 * - Enforce the selected font style to "regular" style
 *
 *  ____________________________
 * |         |       | Styles   |
 * | Family  | Font  | ----     |
 * |  List   | Sizes | Scripts  |
 * |_________|_______|__________|
 * |                            |
 * |   Preview font text line   |
 * |____________________________|
 * |          OK/Cancel buttons |
 * |____________________________|
 *
 * TODO:
 *  - list non-scalable fonts after scalable
 *  - remove the requirement to select size each time
 * TODO?:
 *  - allow translation of "Styles and Scripts"
 *  - add preview of other styles
 */


FontDialog::FontDialog(QWidget *parent)
:QDialog(parent)
{
	// 1. Initialize data holders
	// fontDatabase is declared/initialized in the header file
	regularStyle = "";      // to store the name of the "regular" style

	// 2. Construct all widgets and layouts
	vlayout  = new QVBoxLayout(this);     //top layout
	hlayout  = new QHBoxLayout();           //inside vlayout
	// font family and size lists (with selectable items)
	fontList = new QListWidget(this);     //top sub-widget 1
	sizeList = new QListWidget(this);     //top sub-widget 2

	// style and writing info: three Labels wrapped in GroupBox and ScrollArea
	scrollInfo   = new QScrollArea(this); //top sub-widget 3
	groupboxInfo = new QGroupBox("Styles and Scripts", scrollInfo);
	vlayoutInfo  = new QVBoxLayout(groupboxInfo);  //inside groupboxInfo

	styleInfo     = new QLabel(groupboxInfo);
	separatorInfo = new QLabel(groupboxInfo);
	writingInfo   = new QLabel(groupboxInfo);

	// font preview line
	preview = new QLineEdit(this);        //top sub-widget 4
	// dialog buttons
	buttons = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);   //top sub-widget 5


	// 3. Nest and connect widgets and layouts, set formatting details
	// font family list
	foreach (QString family, fontDatabase.families()) {
		if ( fontDatabase.isFixedPitch(family) ) {
			QListWidgetItem *item = new QListWidgetItem(fontList);
			item->setText(family);
		}
	}
	fontList->setMinimumWidth(80);  // to reduce stretching
	connect(fontList, SIGNAL(itemSelectionChanged()),
			this, SLOT(updateFonts()));
	hlayout->addWidget(fontList,5);
	// font size list (filled later in updateFonts())
	sizeList->setMinimumWidth(40);  // to reduce stretching
	sizeList->setMaximumWidth(60);
	connect(sizeList, SIGNAL(itemSelectionChanged()),
			this, SLOT(fontSelected()));
	hlayout->addWidget(sizeList,1);
	// font family info: styles, separator, scripts (see updateFonts())
	// styles
	styleInfo->setWordWrap(true);
	styleInfo->setAlignment(Qt::AlignTop);
	vlayoutInfo->addWidget(styleInfo);
	// separator
	separatorInfo->setFrameStyle(QFrame::HLine);
	vlayoutInfo->addWidget(separatorInfo);
	// scripts (writing systems)
	writingInfo->setWordWrap(true);
	vlayoutInfo->addWidget(writingInfo);
	groupboxInfo->setLayout(vlayoutInfo);
	scrollInfo->setWidget(groupboxInfo);
	scrollInfo->setWidgetResizable(true);
	scrollInfo->setFocusPolicy(Qt::NoFocus);
	scrollInfo->setMinimumWidth(80);  // to reduce stretching
	hlayout->addWidget(scrollInfo,5);
	vlayout->addLayout(hlayout);
	// font preview line
	preview->setText("The quick brown fox jumps over the lazy dog");
	preview->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
	preview->setEnabled(false);
	preview->setMinimumHeight(40);  // to reduce vertical resizing
	vlayout->addWidget(preview);
	// dialog OK and Cancel buttons
	buttons->button(QDialogButtonBox::Ok)->setEnabled(false);

	connect( buttons->button(QDialogButtonBox::Ok), SIGNAL(clicked()),
			this, SLOT(accept()));
	connect( buttons->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
			this, SLOT(reject()));
	vlayout->addWidget(buttons);

	this->setLayout(vlayout);
}

/** update the list of sizes available for the given family
 *  and the name of the "regular" style in regularStyle
 *  and the styleInfo
 */

void FontDialog::updateFonts()
{
	sizeList->clear();

	QString family;
	if ( fontList->currentItem() == NULL ) {
		return;
	} else {
		family = fontList->currentItem()->text();
	}

	QStringList styles = fontDatabase.styles(family);
	// try to guess which of the styles is the "regular" one
	//int match_idx = styles.indexOf(QRegExp("Regular|Normal|Book|Roman|Plain|Upright|Medium|Light|Sans"));
	int match_idx = styles.indexOf(regular_rx);
	regularStyle = match_idx == -1 ? styles.at(0) : styles.at(match_idx);

	// offer sizes for the regular style
	QList<int> fsizes = fontDatabase.smoothSizes(family, regularStyle);
	if ( fsizes.isEmpty() ) {
		fsizes = fontDatabase.pointSizes(family, regularStyle);
	}
	// or simply all sizes(?), if the above fails
	if ( fsizes.isEmpty() ) {
		fsizes = fontDatabase.standardSizes();
	}

	// fill the size list for the selected family
	foreach ( int points, fsizes ) {
		QListWidgetItem *item = new QListWidgetItem(sizeList);
		item->setText(QString::number(points));
	}

	// scripts (writing systems) supported by the family
	QString wsystems = "";
	foreach ( const QFontDatabase::WritingSystem &writing, fontDatabase.writingSystems(family) ) {
	    wsystems += fontDatabase.writingSystemName(writing) + ", ";
	}
	wsystems.chop(2);   // no trailing comma

	// update the info widgets, buttons and preview
	styleInfo->setText(styles.join("\n") );
	writingInfo->setText(wsystems);

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
			regularStyle, currentS->text().toInt());
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

