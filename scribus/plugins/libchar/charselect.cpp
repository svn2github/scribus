#include "charselect.h"
#include "charselect.moc"
#include "scpainter.h"
#include <qtextcodec.h>
#include <qcursor.h>
#include "config.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H

extern QPixmap loadIcon(QString nam);
 
QString Name()
{
  return "Insert Special";
}

int Type()
{
	return 1;
}

void Run(QWidget *d, ScribusApp *plug)
{
	if ((plug->HaveDoc) && (plug->doc->ActPage->SelItem.count() != 0))
		{
		PageItem *b = plug->doc->ActPage->SelItem.at(0);
		if ((b->PType == 4) && (plug->doc->AppMode == 7))
			{
  		ZAuswahl *dia = new ZAuswahl(d, &plug->Prefs, b, plug);
  		dia->exec();
  		delete dia;
  		}
  	}
}

Zoom::Zoom(QWidget* parent, QPixmap pix) : QDialog( parent, "Edit", false, WStyle_Customize | WStyle_NoBorderEx)
{
	resize(pix.width()+2,pix.height()+2);
	setMinimumSize(QSize(pix.width()+2,pix.height()+2));
	setMaximumSize(QSize(pix.width()+2,pix.height()+2));
	pixm = pix;
}

Zoom::~Zoom()
{
}

void Zoom::paintEvent(QPaintEvent *)
{
	QPainter p;
	p.begin(this);
	p.setPen(black);
	p.setBrush(NoBrush);
  p.drawRect(0, 0, width(), height());
	p.drawPixmap(1, 1, pixm);
	p.end();
}

ChTable::ChTable(ZAuswahl* parent, ScribusApp *pl) : QTable(parent)
{
	Mpressed = false;
	setFocusPolicy(NoFocus);
	ap = pl;
	par = parent;
}

void ChTable::contentsMousePressEvent(QMouseEvent* e)
{
	e->accept();
	int r = rowAt(e->pos().y());
	int c = columnAt(e->pos().x());
	if (e->button() == RightButton)
		{
		Mpressed = true;
		int bh = 48 + qRound(-(*ap->doc->AllFonts)[ap->doc->CurrFont]->numDescender * 48) + 3;
		QPixmap pixm(bh,bh);
		ScPainter *p = new ScPainter(&pixm, bh, bh);
		p->clear();
		pixm.fill(white);
		QWMatrix chma;
		chma.scale(4.8, 4.8);
		FPointArray gly = (*ap->doc->AllFonts)[ap->doc->CurrFont]->GlyphArray[par->Zeich[r*32+c]].Outlines.copy();
		float ww = bh - (*ap->doc->AllFonts)[ap->doc->CurrFont]->CharWidth[par->Zeich[r*32+c]]*48;
		if (gly.size() > 4)
			{
			gly.map(chma);
			p->translate(ww / 2, 1);
			p->setBrush(black);
			p->setFillMode(1);
			p->setupPolygon(&gly);
			p->fillPath();
			p->end();
			}
		delete p;
		dia = new Zoom(this, pixm);
		QPoint ps = QCursor::pos();
		dia->move(ps.x()-2, ps.y()-2);
		dia->show();
		}
}

void ChTable::contentsMouseReleaseEvent(QMouseEvent* e)
{
	e->accept();
	if ((e->button() == RightButton) && (Mpressed))
		{
		Mpressed = false;
		dia->close();
		delete dia;
		}
	if (e->button() == LeftButton)
		emit SelectChar(rowAt(e->pos().y()), columnAt(e->pos().x()));
}
 
ZAuswahl::ZAuswahl( QWidget* parent, preV *Vor, PageItem *item, ScribusApp *pl)
    : QDialog( parent, "ZAuswahl", true, 0 )
{
    setCaption( tr( "Select Character:" )+" "+pl->doc->CurrFont );
    ite = item;
    ap = pl;
  	setIcon(loadIcon("AppIcon.xpm"));
    ZAuswahlLayout = new QVBoxLayout( this );
    ZAuswahlLayout->setSpacing( 6 );
    ZAuswahlLayout->setMargin( 11 );

    ZTabelle = new ChTable( this, pl);
    ZTabelle->setNumCols( 32 );
    ZTabelle->setLeftMargin(0);
    ZTabelle->verticalHeader()->hide();
    ZTabelle->setTopMargin(0);
    ZTabelle->horizontalHeader()->hide();
    ZTabelle->setSorting(false);
    ZTabelle->setSelectionMode(QTable::NoSelection);
    ZTabelle->setColumnMovingEnabled(false);
    ZTabelle->setRowMovingEnabled(false);
    ZTabelle->setFont(pl->doc->UsedFonts[pl->doc->CurrFont]);
		int counter = 1;
		FT_Face face;
		FT_ULong  charcode;
		FT_UInt   gindex;
		face = pl->doc->FFonts[pl->doc->CurrFont];
		charcode = FT_Get_First_Char(face, &gindex );
		while (gindex != 0)
			{
			Zeich.append(charcode);
			counter++;
			charcode = FT_Get_Next_Char(face, charcode, &gindex );
			}
		int ab = counter / 32;
		int ac = counter % 32;
		int cc = 0;
		MaxCount = counter;
		if (ac != 0)
			ab++;
		ZTabelle->setNumRows( ab );
		int bh = 14 + qRound(-(*pl->doc->AllFonts)[pl->doc->CurrFont]->numDescender * 14) + 3;
		QPixmap pixm(bh,bh);
		ScPainter *p = new ScPainter(&pixm, bh, bh);
		p->translate(1,1);
    for (int a = 0; a < ab; a++)
      {
      for (int b = 0; b < 32; b++)
        {
				p->clear();
				pixm.fill(white);
				QWMatrix chma;
				chma.scale(1.4, 1.4);
				FPointArray gly = (*pl->doc->AllFonts)[pl->doc->CurrFont]->GlyphArray[Zeich[cc]].Outlines.copy();
        cc++;
				if (gly.size() > 4)
					{
					gly.map(chma);
					p->setBrush(black);
					p->setFillMode(1);
					p->setupPolygon(&gly);
					p->fillPath();
					p->end();
					}
				QTableItem *it = new QTableItem(ZTabelle, QTableItem::Never, "", pixm);
				ZTabelle->setItem(a, b, it);
        if (cc == counter)
					break;
        }
      }
		delete p;
    for (int d = 0; d < 32; d++)
      {
			ZTabelle->setColumnWidth(d, ZTabelle->rowHeight(0));
      }
    ZTabelle->setMinimumSize(QSize(ZTabelle->rowHeight(0)*32, ZTabelle->rowHeight(0)*7));
    ZAuswahlLayout->addWidget( ZTabelle );

    Zeichen = new QLineEdit( this, "Zeichen" );
    Zeichen->setFont(pl->doc->UsedFonts[pl->doc->CurrFont]);
    ZAuswahlLayout->addWidget( Zeichen );

    Layout1 = new QHBoxLayout;
    Layout1->setSpacing( 6 );
    Layout1->setMargin( 0 );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout1->addItem( spacer );

    Einf = new QPushButton( this, "Einf" );
    Einf->setText( tr( "Insert" ) );
    Layout1->addWidget( Einf );
    QSpacerItem* spacer_2 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout1->addItem( spacer_2 );

    Delete = new QPushButton( this, "Delete" );
    Delete->setText( tr( "Clear" ) );
    Layout1->addWidget( Delete );
    QSpacerItem* spacer_3 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout1->addItem( spacer_3 );

    Close = new QPushButton( this, "Close" );
    Close->setText( tr( "Close" ) );
    Layout1->addWidget( Close );
    QSpacerItem* spacer_4 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout1->addItem( spacer_4 );
    ZAuswahlLayout->addLayout( Layout1 );

    // signals and slots connections
    connect(Close, SIGNAL(clicked()), this, SLOT(accept()));
    connect(Delete, SIGNAL(clicked()), this, SLOT(DelEdit()));
    connect(Einf, SIGNAL(clicked()), this, SLOT(InsChar()));
		connect(ZTabelle, SIGNAL(SelectChar(int, int)), this, SLOT(NeuesZeichen(int, int)));
}

ZAuswahl::~ZAuswahl()
{
}

void ZAuswahl::NeuesZeichen(int r, int c) // , int b, const QPoint &pp)
{
	if ((r*32+c) < MaxCount)
		Zeichen->insert(QChar(Zeich[r*32+c]));
}

void ZAuswahl::DelEdit()
{
	Zeichen->clear();
}

void ZAuswahl::InsChar()
{
	struct Pti *hg;
	QString Tex = Zeichen->text();
	for (uint a=0; a<Tex.length(); a++)
		{
		hg = new Pti;
		hg->ch = Tex.at(a);
		if (hg->ch == QChar(10)) { hg->ch = QChar(13); }
		if (hg->ch == QChar(9)) { hg->ch = " "; }
		hg->cfont = ap->doc->CurrFont;
		hg->csize = ap->doc->CurrFontSize;
		hg->ccolor = ap->doc->CurrTextFill;
		hg->cshade = ap->doc->CurrTextFillSh;
		hg->cstroke = ap->doc->CurrTextStroke;
		hg->cshade2 = ap->doc->CurrTextStrokeSh;
		hg->cscale = ap->doc->CurrTextScale;
		hg->cselect = false;
		hg->cstyle = ap->doc->CurrentStyle;
		hg->cab = ap->doc->CurrentABStil;
		if (ap->doc->Vorlagen[ap->doc->CurrentABStil].Font != "")
			{
			hg->cfont = ap->doc->Vorlagen[ap->doc->CurrentABStil].Font;
			hg->csize = ap->doc->Vorlagen[ap->doc->CurrentABStil].FontSize;
			}
		hg->cextra = 0;
		hg->cselect = false;
		hg->xp = 0;
		hg->yp = 0;
		hg->PRot = 0;
		hg->PtransX = 0;
		hg->PtransY = 0;
		ite->Ptext.insert(ite->CPos, hg);
 		ite->CPos += 1;
		}
	ite->Dirty = true;
	ap->doc->ActPage->update();
	ap->slotDocCh();
}
