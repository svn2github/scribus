/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef CMYKFARBEN_H
#define CMYKFARBEN_H

#include <QDialog>
class QHBoxLayout;
class QVBoxLayout;
class QGridLayout;
class QLabel;
class QFrame;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QStackedWidget;
class QSlider;
class QPixmap;
class QListWidgetItem;
class QTreeWidget;

#include "colorsetmanager.h"
#include "sccolor.h"
#include "scribusapi.h"

class ScColor;
class ScribusDoc;
class ScrSpinBox;
class ScComboBox;
class SwatchCombo;
class ColorListBox;
class ColorChart;

class SCRIBUS_API CMYKChoose : public QDialog
{
	Q_OBJECT

public:
	CMYKChoose( QWidget* parent, ScribusDoc* doc, ScColor orig, QString name, ColorList *Colors, bool newCol );
	~CMYKChoose() {};
	QLabel* TextLabel1;
	QLineEdit* ColorName;
	QLabel* TextLabel3;
	ScComboBox* ComboBox1;
	QCheckBox* Separations;
	SwatchCombo* Swatches;
	QLabel* TextLabel5_2;
	QLabel* OldC;
	QLabel* TextLabel5;
	QLabel* NewC;
	QPushButton* Cancel;
	QPushButton* Cancel_2;
	QFrame* Frame4;
	QStackedWidget* TabStack;
	QFrame* Frame5;
	QFrame* Frame5a;
	ColorListBox* ColorSwatch;
	ColorChart* ColorMap;
	QLabel* CyanP;
	QLabel* CyanT;
	QSlider* CyanSL;
	ScrSpinBox* CyanSp;
	QLabel* MagentaP;
	QLabel* MagentaT;
	QSlider* MagentaSL;
	ScrSpinBox* MagentaSp;
	QLabel* YellowP;
	QLabel* YellowT;
	QSlider* YellowSL;
	ScrSpinBox* YellowSp;
	QLabel* BlackP;
	QLabel* BlackT;
	QSlider* BlackSL;
	ScrSpinBox* BlackSp;
	QPixmap imageA;
	QPixmap imageN;
	QPixmap alertIcon;
	ScColor Farbe;
	bool Wsave;
	bool dynamic;
	bool isNew;
	bool isRegistration;
	int BlackComp;
	ColorList *EColors;
	ColorList CurrSwatch;
	QString Fnam;

public slots:
	void slotRightClick();
	void setValSLiders(double value);
	void setValueS(int val);
	void toggleSL();
	void selSwatch();
	void setSpot();
	void selModel(const QString& mod);
	void setColor();
	void setColor2(int h, int s, bool ende);
	void selFromSwatch(QListWidgetItem* c);
	void setValues();
	void leave();

protected:
	QHBoxLayout* CMYKColorLayout;
	QVBoxLayout* Layout23;
	QGridLayout* Layout2;
	QGridLayout* Layout2x;
	QHBoxLayout* Layout21;
	QVBoxLayout* Frame4Layout;
	QHBoxLayout* Frame5Layout;
	QHBoxLayout* Frame5aLayout;
	QVBoxLayout* Layout1_2;
	QVBoxLayout* Layout1_2_2;
	QVBoxLayout* Layout1_2_3;
	QVBoxLayout* Layout1_2_4;
	
	ColorSetManager csm;
	ScribusDoc* m_doc;
	QTreeWidgetItem *systemSwatches;
	QTreeWidgetItem *userSwatches;
	QTreeWidgetItem *hsvSelector;
	QStringList customColSet;
	
	virtual void showEvent(QShowEvent * event);

	QPixmap sliderPix(int farbe);
	QPixmap sliderBlack();
};

#endif // CMYKFARBEN_H
