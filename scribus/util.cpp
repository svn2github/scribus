/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
                          util.cpp  -  description
                             -------------------
    begin                : Fri Sep 14 2001
    copyright            : (C) 2001 by Franz Schmid
    email                : Franz.Schmid@altmuehlnet.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <QBitmap>
#include <QByteArray>
#include <QCheckBox>
#include <QDomElement>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QList>
#include <QPixmap>
#include <QWidget>
#include <QDir>


#include <cmath>
#include <algorithm>

#include "util.h"

#include "scconfig.h"

#if defined(_WIN32)
#if defined(_MSC_VER)
#define __STDC__ 1 // hack to get md5_buffer correctly identified
#endif
#include <valarray>
#include <windows.h>
#endif

#include "md5.h"

#include "commonstrings.h"
#include "prefsmanager.h"
#include <QProcess>
#include <Q3Process>
#include "scmessagebox.h"
#include "scpixmapcache.h"
#include "scpaths.h"

extern "C"
{
#define XMD_H           // shut JPEGlib up
#if defined(Q_OS_UNIXWARE)
#  define HAVE_BOOLEAN  // libjpeg under Unixware seems to need this
#endif
#include <jpeglib.h>
#include <jerror.h>
#undef HAVE_STDLIB_H
#ifdef const
#  undef const          // remove crazy C hackery in jconfig.h
#endif
}

#include "pageitem.h"
#include "scribus.h"
#include "scribusdoc.h"
#include "scribusview.h"
#include <zlib.h>


using namespace std;

void sDebug(QString message)
{
	qDebug("%s", message.ascii());
}

int System(const QString exename, const QStringList & args, const QString fileStdErr, const QString fileStdOut)
{
	QStringList stdErrData;
	QStringList stdOutData;
	QStringList exeArgs;
	exeArgs<<exename<<args;
	Q3Process proc(exeArgs);
	if ( !proc.start() )
		return 1;
	/* start was OK */
	/* wait a little bit */
	while( proc.isRunning() || proc.canReadLineStdout() || proc.canReadLineStderr() )
	{
		// Otherwise Scribus will sleep a *lot* when proc has huge std output
		if ( !proc.canReadLineStdout() && !proc.canReadLineStderr()) {
#ifndef _WIN32
			usleep(5000);
#else
			Sleep(5);
#endif
		}
		// Some configurations needs stdout and stderr to be read
		// if needed before the created process can exit
		if ( proc.canReadLineStdout() ) 
			stdOutData.append( proc.readLineStdout() ); 
		if ( proc.canReadLineStderr() ) 
			stdErrData.append( proc.readLineStderr() ); 
	}
	// TODO: What about proc.normalExit() ?
	int ex = proc.exitStatus();
	QStringList::iterator pIterator;
	QStringList::iterator pEnd;
	if ( !fileStdErr.isEmpty() )
	{
		QFile ferr(fileStdErr);
		if ( ferr.open(QIODevice::WriteOnly) )
		{
			pEnd = stdErrData.end();
			QTextStream errStream(&ferr);
			for ( pIterator = stdErrData.begin(); pIterator != pEnd; pIterator++ )
				errStream << *pIterator << endl;
			ferr.close();
		}
	}

	if ( !fileStdOut.isEmpty() )
	{
		QFile fout(fileStdOut);
		if ( fout.open(QIODevice::WriteOnly) )
		{
			pEnd = stdOutData.end();
			QTextStream outStream(&fout);
			for ( pIterator = stdOutData.begin(); pIterator != pEnd; pIterator++ )
				outStream << *pIterator << endl;
			fout.close();
		}
	}
	return ex;
}

// On Windows, return short path name, else return longPath;
QString getShortPathName(QString longPath)
{
	QString shortPath(longPath);
#if defined _WIN32
	QFileInfo fInfo(longPath);
	if(fInfo.exists())
	{
		char shortName[MAX_PATH + 1];
		// An error should not be blocking as ERROR_INVALID_PARAMETER can simply mean
		// that volume does not support 8.3 filenames, so return longPath in this case
		int ret = GetShortPathName(QDir::convertSeparators(longPath).local8Bit(), shortName, sizeof(shortName));
		if( ret != ERROR_INVALID_PARAMETER && ret < sizeof(shortName))
			shortPath = shortName;
	}
#endif
	return shortPath;
}

int copyFile(QString source, QString target)
{
	int bytesread;
	if ((source.isNull()) || (target.isNull()))
		return -1;
	if (source == target)
		return -1;
	QFile s(source);
	if (!s.exists())
		return -1;
	QFile t(target);
	QByteArray bb( 65536 );
	if (s.open(QIODevice::ReadOnly))
	{
		if (t.open(QIODevice::WriteOnly))
		{
			bytesread = s.readBlock( bb.data(), bb.size() );
			while( bytesread > 0 )
			{
				t.writeBlock( bb.data(), bytesread );
				bytesread = s.readBlock( bb.data(), bb.size() );
			}
			t.close();
		}
		s.close();
	}
	return 0;
}

int moveFile(QString source, QString target)
{
	if ((source.isNull()) || (target.isNull()))
		return -1;
	if (source == target)
		return -1;
	copyFile(source, target);
	QFile::remove(source);
	return 0;
}

QString GetAttr(QDomElement *el, QString at, QString def)
{
	return el->attribute(at, def);
}


// Legacy implementation of LoadText with incorrect
// handling of unicode data. This should be retired.
// Use loadRawText instead.
// FIXME XXX
//
bool loadText(QString filename, QString *Buffer)
{
	QFile f(filename);
	QFileInfo fi(f);
	if (!fi.exists())
		return false;
	bool ret;
	QByteArray bb(f.size());
	if (f.open(QIODevice::ReadOnly))
	{
		f.readBlock(bb.data(), f.size());
		f.close();
		for (int posi = 0; posi < bb.size(); ++posi)
			*Buffer += QChar(bb[posi]);
		/*
		int len = bb.size();
		int oldLen = Buffer->length();
		Buffer->setLength( oldLen + len + 1);
		// digged into Qt 3.3 sources to find that. Might break in Qt 4 -- AV
		unsigned short * ucsString = const_cast<unsigned short *>(Buffer->ucs2()) + oldLen;
		char * data = bb.data();
		for (uint posi = 0; posi < len; ++posi)
		*ucsString++ = *data++;
		*ucsString = 0;
		*/
		ret = true;
	}
	else
		ret = false;
	return ret;
}

bool loadRawText(const QString & filename, QByteArray & buf)
{
	bool ret = false;
	QFile f(filename);
	QFileInfo fi(f);
	if (fi.exists())
	{
		QByteArray tempBuf(f.size() + 1);
		if (f.open(QIODevice::ReadOnly))
		{
			unsigned int bytesRead = f.readBlock(tempBuf.data(), f.size());
			tempBuf[bytesRead] = '\0';
			ret = bytesRead == f.size();
			if (ret)
				buf = tempBuf; // sharing makes this efficient
		}
	}
	if (f.isOpen())
		f.close();
	return ret;
}

bool loadRawBytes(const QString & filename, QByteArray & buf)
{
	bool ret = false;
	QFile f(filename);
	QFileInfo fi(f);
	if (fi.exists())
	{
		QByteArray tempBuf(f.size());
		if (f.open(QIODevice::ReadOnly))
		{
			unsigned int bytesRead = f.readBlock(tempBuf.data(), f.size());
			ret = bytesRead == f.size();
			if (ret)
				buf = tempBuf; // sharing makes this efficient
		}
	}
	if (f.isOpen())
		f.close();
	return ret;
}


QString CompressStr(QString *in)
{
	QString out = "";
	QByteArray bb(in->length());
	if (bb.size() == in->length())
	{
		for (int ax = 0; ax < in->length(); ++ax)
		{
			// bb.insert(ax, in->at(ax)); JG monstruously inefficient due to frequent memory reallocation
			bb[ax] = in->at(ax).cell();
			assert(in->at(ax).row() == 0);
		}
		uLong exlen = (uLong)(bb.size() * 0.001 + 16) + bb.size();
		QByteArray bc(exlen);
		if( bc.size() == static_cast<qint32>(exlen) )
		{
			int errcode = compress2((Byte *)bc.data(), &exlen, (Byte *)bb.data(), uLong(bb.size()), 9);
			if (errcode != Z_OK)
			{
				qDebug("compress2 failed with code %i", errcode);
				out = *in;
			}
			else {
				for (uint cl = 0; cl < exlen; ++cl)
					out += QChar(bc[cl]);
			}
		}
		else
		{
			qDebug("insufficient memory to allocate %i bytes", in->length());
			out = *in;
		}
	}
	else
	{
		qDebug("insufficient memory to allocate %i bytes", in->length());
		out = *in;
	}
	return out;
}

QByteArray CompressArray(QByteArray *in)
{
	QByteArray out;
	uLong exlen = uint(in->size() * 0.001 + 16) + in->size();
	QByteArray temp(exlen);
	int errcode = compress2((Byte *)temp.data(), &exlen, (Byte *)in->data(), uLong(in->size()), 9);
	if (errcode != Z_OK)
	{
		qDebug("compress2 failed with code %i", errcode);
		out = *in;
	}
	else {
		temp.resize(exlen);
		out = temp;
	}
	return out;
}

char *toHex( uchar u )
{
	static char hexVal[3];
	int i = 1;
	while ( i >= 0 )
	{
		ushort hex = (u & 0x000f);
		if ( hex < 0x0a )
			hexVal[i] = '0'+hex;
		else
			hexVal[i] = 'A'+(hex-0x0a);
		u = u >> 4;
		i--;
	}
	hexVal[2] = '\0';
	return hexVal;
}

QString String2Hex(QString *in, bool lang)
{
	int i = 0;
	QString out("");
	for( int xi = 0; xi < in->length(); ++xi )
	{
		// Qt4 .cell() added ???
		out += toHex(QChar(in->at(xi)).cell());
		++i;
		if ((i>40) && (lang))
		{
			out += '\n';
			i=0;
		}
	}
	return out;
}

QByteArray ComputeMD5Sum(QByteArray *in)
{
	QByteArray MDsum(16);
	md5_buffer (in->data(), in->size(), reinterpret_cast<void*>(MDsum.data()));
	return MDsum;
}

QString Path2Relative(QString Path, const QString& baseDir)
{
	QDir d(baseDir);
	return d.relativeFilePath(Path);
}

QString Relative2Path(QString File, const QString& baseDir)
{
	QString   absPath;
	QFileInfo fi(File);
	if (File.isEmpty())
		absPath = File;
	else if (fi.isRelative())
	{
		QDir d(baseDir);
		absPath = d.absoluteFilePath(File);
		absPath = QDir::cleanPath(absPath);
	}
	else
		absPath = File;
	return absPath;
}

/***************************************************************************
    begin                : Wed Oct 29 2003
    copyright            : (C) 2003 The Scribus Team
    email                : paul@all-the-johnsons.co.uk
 ***************************************************************************/
// check if the file exists, if it does, ask if they're sure
// return true if they're sure, else return false;

bool overwrite(QWidget *parent, QString filename)
{
	bool retval = true;
	QFileInfo fi(filename);
	if (fi.exists())
	{
		QString fn = QDir::convertSeparators(filename);
		int t = QMessageBox::warning(parent, QObject::tr("File exists"),
									 "<qt>"+ QObject::tr("A file named '%1' already exists.<br/>Do you want to replace it with the file you are saving?").arg(fn) +"</qt>",
											 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
		if (t == QMessageBox::Cancel)
			retval = false;
	}
	return retval;
}

void WordAndPara(PageItem* currItem, int *w, int *p, int *c, int *wN, int *pN, int *cN)
{
	QChar Dat = QChar(32);
	int para = 0;
	int ww = 0;
	int cc = 0;
	int paraN = 0;
	int wwN = 0;
	int ccN = 0;
	bool first = true;
	PageItem *nextItem = currItem;
	PageItem *nbl = currItem;
	while (nextItem != 0)
	{
		if (nextItem->prevInChain() != 0)
			nextItem = nextItem->prevInChain();
		else
			break;
	}
	while (nextItem != 0)
	{
		for (int a = qMax(nextItem->firstInFrame(),0); a <= nextItem->lastInFrame() && a < nextItem->itemText.length(); ++a)
		{
			QChar b = nextItem->itemText.text(a);
			if (b == SpecialChars::PARSEP)
			{
				para++;
			}
			if ((!b.isLetterOrNumber()) && (Dat.isLetterOrNumber()) && (!first))
			{
				ww++;
			}
			cc++;
			Dat = b;
			first = false;
		}
		nbl = nextItem;
		nextItem = nextItem->nextInChain();
	}
	if (nbl->frameOverflows()) {
		paraN++;
		for (int a = nbl->lastInFrame()+1; a < nbl->itemText.length(); ++a)
		{
			QChar b = nbl->itemText.text(a);
			if (b == SpecialChars::PARSEP)
			{
				paraN++;
			}
			if ((!b.isLetterOrNumber()) && (Dat.isLetterOrNumber()) && (!first))
			{
				wwN++;
			}
			ccN++;
			Dat = b;
			first = false;
		}
	}		
	else {
		para++;
	}
	if (Dat.isLetterOrNumber())
	{
		if (nbl->frameOverflows())
			wwN++;
		else
			ww++;
	}
	*w = ww;
	*p = para;
	*c = cc;
	*wN = wwN;
	*pN = paraN;
	*cN = ccN;
}

void ReOrderText(ScribusDoc *currentDoc, ScribusView *view)
{
	double savScale = view->scale();
	view->setScale(1.0);
	currentDoc->RePos = true;
	QImage pgPix(10, 10, QImage::Format_ARGB32);
	QRect rd = QRect(0,0,9,9);
	ScPainter *painter = new ScPainter(&pgPix, pgPix.width(), pgPix.height());
	for (int azz=0; azz<currentDoc->MasterItems.count(); ++azz)
	{
		PageItem *currItem = currentDoc->MasterItems.at(azz);
		if (currItem->itemType() == PageItem::PathText)
			currItem->DrawObj(painter, rd);
	}
	for (int azz=0; azz<currentDoc->Items->count(); ++azz)
	{
		PageItem *currItem = currentDoc->Items->at(azz);
		if (currItem->itemType() == PageItem::TextFrame)
			currItem->asTextFrame()->layout();
		else if (currItem->itemType() == PageItem::PathText)
			currItem->DrawObj(painter, rd);
	}
	currentDoc->RePos = false;
	view->setScale(savScale);
	delete painter;
}

/*! \brief Helper function for sorting in sortQStringList.
\author 10/06/2004 - pv
\param s1 first string
\param s2 second string
\retval bool t/f related s1>s2
 */
bool compareQStrings(QString s1, QString s2)
{
	if (QString::localeAwareCompare(s1, s2) >= 0)
		return false;
	return true;
}

QStringList sortQStringList(QStringList aList)
{
	std::vector<QString> sortList;
	QStringList retList;
	QStringList::Iterator it;
	for (it = aList.begin(); it != aList.end(); ++it)
		sortList.push_back(*it);
	std::sort(sortList.begin(), sortList.end(), compareQStrings);
	for(uint i = 0; i < sortList.size(); i++)
		retList.append(sortList[i]);
	return retList;
}

void GetItemProps(bool newVersion, QDomElement *obj, struct CopyPasteBuffer *OB, const QString& baseDir)
{
	QString tmp;
	int x, y;
	double xf, yf, xf2;
	OB->PType = static_cast<PageItem::ItemType>(obj->attribute("PTYPE").toInt());
	OB->Width=obj->attribute("WIDTH").toDouble();
	OB->Height=obj->attribute("HEIGHT").toDouble();
	OB->RadRect = obj->attribute("RADRECT", "0").toDouble();
	OB->ClipEdited = obj->attribute("CLIPEDIT", "0").toInt();
	OB->FrameType = obj->attribute("FRTYPE", "0").toInt();
	OB->Pwidth=obj->attribute("PWIDTH").toDouble();
	OB->Pcolor = obj->attribute("PCOLOR");
	if ((!newVersion) && (OB->PType == 4))
	{
		OB->TxtFill = obj->attribute("PCOLOR2");
		OB->Pcolor2 = CommonStrings::None;
	}
	else
	{
		OB->Pcolor2 = obj->attribute("PCOLOR2");
		OB->TxtFill = obj->attribute("TXTFILL", "Black");
	}
	OB->Shade = obj->attribute("SHADE").toInt();
	OB->Shade2 = obj->attribute("SHADE2").toInt();
	OB->FillRule = obj->attribute("fillRule", "1").toInt();
	OB->TxtStroke=obj->attribute("TXTSTROKE", CommonStrings::None);
	OB->ShTxtFill=obj->attribute("TXTFILLSH", "100").toInt();
	OB->ShTxtStroke=obj->attribute("TXTSTRSH", "100").toInt();
	OB->TxtScale=qRound(obj->attribute("TXTSCALE", "100").toDouble() * 10);
	OB->TxtScaleV=qRound(obj->attribute("TXTSCALEV", "100").toDouble() * 10);
	OB->TxTBase=qRound(obj->attribute("TXTBASE", "0").toDouble() * 10);
	OB->TxTStyle=obj->attribute("TXTSTYLE", "0").toInt();
	OB->TxtShadowX=qRound(obj->attribute("TXTSHX", "5").toDouble() * 10);
	OB->TxtShadowY=qRound(obj->attribute("TXTSHY", "-5").toDouble() * 10);
	OB->TxtOutline=qRound(obj->attribute("TXTOUT", "1").toDouble() * 10);
	OB->TxtUnderPos=qRound(obj->attribute("TXTULP", "-0.1").toDouble() * 10);
	OB->TxtUnderWidth=qRound(obj->attribute("TXTULW", "-0.1").toDouble() * 10);
	OB->TxtStrikePos=qRound(obj->attribute("TXTSTP", "-0.1").toDouble() * 10);
	OB->TxtStrikeWidth=qRound(obj->attribute("TXTSTW", "-0.1").toDouble() * 10);
	OB->Cols = obj->attribute("COLUMNS", "1").toInt();
	OB->ColGap = obj->attribute("COLGAP", "0.0").toDouble();
	OB->GrType = obj->attribute("GRTYP", "0").toInt();
	OB->fill_gradient.clearStops();
	if (OB->GrType != 0)
	{
		if (OB->GrType == 8)
		{
			OB->pattern = obj->attribute("pattern", "");
			OB->patternScaleX = obj->attribute("pScaleX", "100.0").toDouble();
			OB->patternScaleY = obj->attribute("pScaleY", "100.0").toDouble();
			OB->patternOffsetX = obj->attribute("pOffsetX", "0.0").toDouble();
			OB->patternOffsetY = obj->attribute("pOffsetY", "0.0").toDouble();
			OB->patternRotation = obj->attribute("pRotation", "0.0").toDouble();
		}
		else
		{
			OB->GrStartX = obj->attribute("GRSTARTX", "0.0").toDouble();
			OB->GrStartY = obj->attribute("GRSTARTY", "0.0").toDouble();
			OB->GrEndX = obj->attribute("GRENDX", "0.0").toDouble();
			OB->GrEndY = obj->attribute("GRENDY", "0.0").toDouble();
			OB->GrColor = obj->attribute("GRCOLOR","");
			if (OB->GrColor.isEmpty())
				OB->GrColor = "Black";
			OB->GrColor2 = obj->attribute("GRCOLOR2","Black");
			if (OB->GrColor2.isEmpty())
				OB->GrColor2 = "Black";
			OB->GrShade = obj->attribute("GRSHADE", "100").toInt();
			OB->GrShade2 = obj->attribute("GRSHADE2", "100").toInt();
		}
	}
	OB->Rot=obj->attribute("ROT").toDouble();
	OB->PLineArt=Qt::PenStyle(obj->attribute("PLINEART").toInt());
	OB->PLineEnd=Qt::PenCapStyle(obj->attribute("PLINEEND", "0").toInt());
	OB->PLineJoin=Qt::PenJoinStyle(obj->attribute("PLINEJOIN", "0").toInt());
	OB->LineSp=obj->attribute("LINESP").toDouble();
	OB->LineSpMode = obj->attribute("LINESPMode", "0").toInt();
	OB->LocalScX=obj->attribute("LOCALSCX").toDouble();
	OB->LocalScY=obj->attribute("LOCALSCY").toDouble();
	OB->LocalX=obj->attribute("LOCALX").toDouble();
	OB->LocalY=obj->attribute("LOCALY").toDouble();
	OB->PicArt=obj->attribute("PICART").toInt();
	OB->flippedH = obj->attribute("FLIPPEDH").toInt() % 2;
	OB->flippedV = obj->attribute("FLIPPEDV").toInt() % 2;
/*	OB->BBoxX=obj->attribute("BBOXX").toDouble();
	OB->BBoxH=obj->attribute("BBOXH").toDouble(); */
	OB->ScaleType = obj->attribute("SCALETYPE", "1").toInt();
	OB->AspectRatio = obj->attribute("RATIO", "0").toInt();
	OB->isPrintable=obj->attribute("PRINTABLE").toInt();
	OB->m_isAnnotation=obj->attribute("ANNOTATION", "0").toInt();
	OB->m_annotation.setType(obj->attribute("ANTYPE", "0").toInt());
	OB->m_annotation.setAction(obj->attribute("ANACTION",""));
	OB->m_annotation.setE_act(obj->attribute("ANEACT",""));
	OB->m_annotation.setX_act(obj->attribute("ANXACT",""));
	OB->m_annotation.setD_act(obj->attribute("ANDACT",""));
	OB->m_annotation.setFo_act(obj->attribute("ANFOACT",""));
	OB->m_annotation.setBl_act(obj->attribute("ANBLACT",""));
	OB->m_annotation.setK_act(obj->attribute("ANKACT",""));
	OB->m_annotation.setF_act(obj->attribute("ANFACT",""));
	OB->m_annotation.setV_act(obj->attribute("ANVACT",""));
	OB->m_annotation.setC_act(obj->attribute("ANCACT",""));
	OB->m_annotation.setActionType(obj->attribute("ANACTYP", "0").toInt());
	OB->m_annotation.setExtern(obj->attribute("ANEXTERN",""));
	if ((!OB->m_annotation.Extern().isEmpty()) && (OB->m_annotation.ActionType() != 8))
	{
		QFileInfo efp(OB->m_annotation.Extern());
		OB->m_annotation.setExtern(efp.absFilePath());
	}
	OB->m_annotation.setZiel(obj->attribute("ANZIEL", "0").toInt());
	OB->AnName=obj->attribute("ANNAME","");
	OB->m_annotation.setToolTip(obj->attribute("ANTOOLTIP",""));
	OB->m_annotation.setRollOver(obj->attribute("ANROLL",""));
	OB->m_annotation.setDown(obj->attribute("ANDOWN",""));
	OB->m_annotation.setBwid(obj->attribute("ANBWID", "1").toInt());
	OB->m_annotation.setBsty(obj->attribute("ANBSTY", "0").toInt());
	OB->m_annotation.setFeed(obj->attribute("ANFEED", "1").toInt());
	OB->m_annotation.setFlag(obj->attribute("ANFLAG", "0").toInt());
	OB->m_annotation.setFont(obj->attribute("ANFONT", "4").toInt());
	OB->m_annotation.setFormat(obj->attribute("ANFORMAT", "0").toInt());
	OB->m_annotation.setVis(obj->attribute("ANVIS", "0").toInt());
	OB->m_annotation.setIsChk(static_cast<bool>(obj->attribute("ANCHK", "0").toInt()));
	OB->m_annotation.setAAact(static_cast<bool>(obj->attribute("ANAA", "0").toInt()));
	OB->m_annotation.setHTML(static_cast<bool>(obj->attribute("ANHTML", "0").toInt()));
	OB->m_annotation.setUseIcons(static_cast<bool>(obj->attribute("ANICON", "0").toInt()));
	OB->m_annotation.setChkStil(obj->attribute("ANCHKS", "0").toInt());
	OB->m_annotation.setMaxChar(obj->attribute("ANMC", "-1").toInt());
	OB->m_annotation.setBorderColor(obj->attribute("ANBCOL",CommonStrings::None));
	OB->m_annotation.setIPlace(obj->attribute("ANPLACE", "1").toInt());
	OB->m_annotation.setScaleW(obj->attribute("ANSCALE", "0").toInt());
	if (obj->attribute("TRANSPARENT", "0").toInt() == 1)
		OB->Pcolor = CommonStrings::None;
	OB->textAlignment=obj->attribute("ALIGN", "0").toInt();
	if ( obj->hasAttribute("TEXTFLOWMODE") )
		OB->TextflowMode = (PageItem::TextFlowMode) obj->attribute("TEXTFLOWMODE", "0").toInt();
	else if ( obj->attribute("TEXTFLOW").toInt() )
	{
		if (obj->attribute("TEXTFLOW2", "0").toInt())
			OB->TextflowMode = PageItem::TextFlowUsesBoundingBox;
		else if (obj->attribute("TEXTFLOW3", "0").toInt())
			OB->TextflowMode = PageItem::TextFlowUsesContourLine;
		else
			OB->TextflowMode = PageItem::TextFlowUsesFrameShape;	
	}
	else
		OB->TextflowMode = PageItem::TextFlowDisabled;
	OB->Extra=obj->attribute("EXTRA").toDouble();
	OB->TExtra=obj->attribute("TEXTRA", "1").toDouble();
	OB->BExtra=obj->attribute("BEXTRA", "1").toDouble();
	OB->RExtra=obj->attribute("REXTRA", "1").toDouble();
	OB->PoShow = obj->attribute("PLTSHOW", "0").toInt();
	OB->BaseOffs = obj->attribute("BASEOF", "0").toDouble();
	OB->textPathType =  obj->attribute("textPathType", "0").toInt();
	OB->textPathFlipped = static_cast<bool>(obj->attribute("textPathFlipped", "0").toInt());
	OB->ISize = qRound(obj->attribute("ISIZE", "12").toDouble() * 10);
	if (obj->hasAttribute("EXTRAV"))
		OB->ExtraV = qRound(obj->attribute("EXTRAV", "0").toDouble() / obj->attribute("ISIZE", "12").toDouble() * 1000.0);
	else
		OB->ExtraV = obj->attribute("TXTKERN").toInt();
	OB->Pfile  = Relative2Path(obj->attribute("PFILE" ,""), baseDir);
	OB->Pfile2 = Relative2Path(obj->attribute("PFILE2",""), baseDir);
	OB->Pfile3 = Relative2Path(obj->attribute("PFILE3",""), baseDir);
	OB->IProfile=obj->attribute("PRFILE","");
	OB->EmProfile=obj->attribute("EPROF","");
	OB->IRender = obj->attribute("IRENDER", "1").toInt();
	OB->UseEmbedded = obj->attribute("EMBEDDED", "1").toInt();
	OB->Locked = static_cast<bool>(obj->attribute("LOCK", "0").toInt());
	OB->LockRes = static_cast<bool>(obj->attribute("LOCKR", "0").toInt());
	OB->Reverse = static_cast<bool>(obj->attribute("REVERS", "0").toInt());
	OB->isTableItem = static_cast<bool>(obj->attribute("isTableItem", "0").toInt());
	OB->TopLine = static_cast<bool>(obj->attribute("TopLine", "0").toInt());
	OB->LeftLine = static_cast<bool>(obj->attribute("LeftLine", "0").toInt());
	OB->RightLine = static_cast<bool>(obj->attribute("RightLine", "0").toInt());
	OB->BottomLine = static_cast<bool>(obj->attribute("BottomLine", "0").toInt());
	OB->TopLinkID =  obj->attribute("TopLINK", "-1").toInt();
	OB->LeftLinkID =  obj->attribute("LeftLINK", "-1").toInt();
	OB->RightLinkID =  obj->attribute("RightLINK", "-1").toInt();
	OB->BottomLinkID =  obj->attribute("BottomLINK", "-1").toInt();
	OB->Transparency = obj->attribute("TransValue", "0.0").toDouble();
	if (obj->hasAttribute("TransValueS"))
		OB->TranspStroke = obj->attribute("TransValueS", "0.0").toDouble();
	else
		OB->TranspStroke = OB->Transparency;
	OB->TransBlend = obj->attribute("TransBlend", "0").toInt();
	OB->TransBlendS = obj->attribute("TransBlendS", "0").toInt();
	tmp = "";
	if (obj->hasAttribute("NUMCLIP"))
	{
		OB->Clip.resize(obj->attribute("NUMCLIP").toUInt());
		tmp = obj->attribute("CLIPCOOR");
		QTextStream fc(&tmp, QIODevice::ReadOnly);
		for (uint c=0; c<obj->attribute("NUMCLIP").toUInt(); ++c)
		{
			fc >> x;
			fc >> y;
			OB->Clip.setPoint(c, x, y);
		}
	}
	else
		OB->Clip.resize(0);
	tmp = "";
	if (obj->hasAttribute("NUMPO"))
	{
		OB->PoLine.resize(obj->attribute("NUMPO").toUInt());
		tmp = obj->attribute("POCOOR");
		QTextStream fp(&tmp, QIODevice::ReadOnly);
		for (uint cx=0; cx<obj->attribute("NUMPO").toUInt(); ++cx)
		{
			fp >> xf;
			fp >> yf;
			OB->PoLine.setPoint(cx, xf, yf);
		}
	}
	else
		OB->PoLine.resize(0);
	tmp = "";
	if (obj->hasAttribute("NUMCO"))
	{
		OB->ContourLine.resize(obj->attribute("NUMCO").toUInt());
		tmp = obj->attribute("COCOOR");
		QTextStream fp(&tmp, QIODevice::ReadOnly);
		for (uint cx=0; cx<obj->attribute("NUMCO").toUInt(); ++cx)
		{
			fp >> xf;
			fp >> yf;
			OB->ContourLine.setPoint(cx, xf, yf);
		}
	}
	else
		OB->ContourLine.resize(0);
	tmp = "";
	if ((obj->hasAttribute("NUMTAB")) && (obj->attribute("NUMTAB", "0").toInt() != 0))
	{
		ParagraphStyle::TabRecord tb;
		tmp = obj->attribute("TABS");
		QTextStream tgv(&tmp, QIODevice::ReadOnly);
		OB->TabValues.clear();
		for (int cxv = 0; cxv < obj->attribute("NUMTAB", "0").toInt(); cxv += 2)
		{
			tgv >> xf;
			tgv >> xf2;
			tb.tabPosition = xf2;
			tb.tabType = static_cast<int>(xf);
			tb.tabFillChar = QChar();
			OB->TabValues.append(tb);
		}
		tmp = "";
	}
	else
		OB->TabValues.clear();
	if ((obj->hasAttribute("NUMDASH")) && (obj->attribute("NUMDASH", "0").toInt() != 0))
	{
		tmp = obj->attribute("DASHS");
		QTextStream dgv(&tmp, QIODevice::ReadOnly);
		OB->DashValues.clear();
		for (int cxv = 0; cxv < obj->attribute("NUMDASH", "0").toInt(); ++cxv)
		{
			dgv >> xf;
			OB->DashValues.append(xf);
		}
		tmp = "";
	}
	else
		OB->DashValues.clear();
	OB->DashOffset = obj->attribute("DASHOFF", "0.0").toDouble();
}

QString checkFileExtension(const QString &currName, const QString &extension)
{
	QString newName(currName);
	//If filename ends with a period, just add the extension
	if (newName.right(1)==".")
	{
		newName+=extension.lower();
		return newName;
	}
	//If filename doesnt end with the period+extension, add it on
	QString dotExt("." + extension.lower());
	if (!newName.endsWith(dotExt,false))
		newName+=dotExt;
	return newName;
}

QString getFileNameByPage(ScribusDoc* currDoc, uint pageNo, QString extension)
{
	uint number = pageNo + currDoc->FirstPnum;
	QString defaultName = currDoc->DocName;
	if (defaultName.isNull())
		defaultName = "export";
	else
	{
		QFileInfo fi(defaultName);
		defaultName = fi.baseName(true);
	}
	return QString("%1-%2%3.%4").arg(defaultName).arg(QObject::tr("page", "page export")).arg(number, 3, 10, QChar('0')).arg(extension);
}

const QString getStringFromSequence(DocumentSectionType type, uint position)
{
	QString retVal("");
	switch( type )
	{
		case Type_1_2_3:
			retVal=QString::number(position);
			break;
		case Type_A_B_C:
			retVal=numberToLetterSequence(position).upper();
			break;
		case Type_a_b_c:
			retVal=numberToLetterSequence(position);
			break;
		case Type_I_II_III:
			retVal=arabicToRoman(position);
			break;
		case Type_i_ii_iii:
			//well, for lower case people will want that, even if its "wrong"
			//ie, X=10, x=10000
			retVal=arabicToRoman(position).lower();
			break;
		case Type_None:
			break;
		default:
			break;
	}
	return retVal;
}

const QString numberToLetterSequence(uint i)
{
	QString retVal("");
	unsigned digits = 1;
	unsigned offset = 0;
	uint column=i;
	--column;

	if( column > 4058115285U ) return  QString("@");

	for( unsigned limit = 26; column >= limit+offset; limit *= 26, digits++ )
		offset += limit;

	for( unsigned c = column - offset; digits; --digits, c/=26 )
		retVal.prepend( QChar( 'a' + (c%26) ) );
	return retVal;
}

const QString arabicToRoman(uint i)
{
	QString roman("");
	int arabic = i;
	while (arabic - 1000000 >= 0){
		roman += "m";
		arabic -= 1000000;
	}
	while (arabic - 900000 >= 0){
		roman += "cm";
		arabic -= 900000;
	}
	while (arabic - 500000 >= 0){
		roman += "d";
		arabic -= 500000;
	}
	while (arabic - 400000 >= 0){
		roman += "cd";
		arabic -= 400000;
	}
	while (arabic - 100000 >= 0){
		roman += "c";
		arabic -= 100000;
	}
	while (arabic - 90000 >= 0){
		roman += "xc";
		arabic -= 90000;
	}
	while (arabic - 50000 >= 0){
		roman += "l";
		arabic -= 50000;
	}
	while (arabic - 40000 >= 0){
		roman += "xl";
		arabic -= 40000;
	}
	while (arabic - 10000 >= 0){
		roman += "x";
		arabic -= 10000;
	}
	while (arabic - 9000 >= 0){
		roman += "Mx";
		arabic -= 9000;
	}
	while (arabic - 5000 >= 0){
		roman += "v";
		arabic -= 5000;
	}
	while (arabic - 4000 >= 0){
		roman += "Mv";
		arabic -= 4000;
	}
	while (arabic - 1000 >= 0){
		roman += "M";
		arabic -= 1000;
	}
	while (arabic - 900 >= 0){
		roman += "CM";
		arabic -= 900;
	}
	while (arabic - 500 >= 0){
		roman += "D";
		arabic -= 500;
	}
	while (arabic - 400 >= 0){
		roman += "CD";
		arabic -= 400;
	}
	while (arabic - 100 >= 0){
		roman += "C";
		arabic -= 100;
	}
	while (arabic - 90 >= 0){
		roman += "XC";
		arabic -= 90;
	}
	while (arabic - 50 >= 0){
		roman += "L";
		arabic -= 50;
	}
	while (arabic - 40 >= 0){
		roman += "XL";
		arabic -= 40;
	}
	while (arabic - 10 >= 0){
		roman += "X";
		arabic -= 10;
	}
	while (arabic - 9 >= 0){
		roman += "IX";
		arabic -= 9;
	}
	while (arabic - 5 >= 0){
		roman += "V";
		arabic -= 5;
	}
	while (arabic - 4 >= 0){
		roman += "IV";
		arabic -= 4;
	}
	while (arabic - 1 >= 0){
		roman += "I";
		arabic -= 1;
	}
	return roman;
}

//CB Moved from scribus.cpp
void parsePagesString(QString pages, std::vector<int>* pageNs, int sourcePageCount)
{
	QString tmp(pages);
	QString token;
	int from, to, pageNr;
	do
	{
		if (tmp.find(",") == -1)
		{
			token = tmp;
			tmp = "";
		}
		else
		{
			token = tmp.left(tmp.find(","));
			tmp = tmp.right(tmp.length() - tmp.find(",") - 1);
		}

		token = token.stripWhiteSpace();
		if (token == "*") // Import all source doc pages
		{
			for (int i = 1; i <= sourcePageCount; ++i)
				pageNs->push_back(i);
		}
		else if (token.find("-") != -1) // import a range of source doc pages
		{
			from = QString(token.left(token.find("-"))).toInt();
			to = QString(token.right(token.length() - token.find("-") - 1)).toInt();
			if ((from != 0) && (to != 0))
			{
				if (from > sourcePageCount)
					from = sourcePageCount;
				if (to > sourcePageCount)
					to = sourcePageCount;
				if (from == to)
					pageNs->push_back(to);
				else if (from < to)
				{
					for (int i = from; i <= to; ++i)
						pageNs->push_back(i);
				}
				else
				{
					for (int i = from; i >= to; --i)
						pageNs->push_back(i);
				}
			}
		}
		else // import single source doc page
		{
			pageNr = token.toInt();
			if ((pageNr > 0) && (pageNr <= sourcePageCount))
				pageNs->push_back(pageNr);
		}
	} while (!tmp.isEmpty());
}


int findParagraphStyle(ScribusDoc* doc, const ParagraphStyle& parStyle)
{
	bool named = !parStyle.name().isEmpty();
//qDebug(QString("looking up %1/ %2").arg(parStyle.name()).arg(parStyle.alignment())); 
	if (named) {
		for (uint i=0; i < doc->paragraphStyles().count(); ++i)
		{
//qDebug(QString("%1 %2").arg(i).arg(doc->paragraphStyles()[i].name()));
			if (parStyle.name() == doc->paragraphStyles()[i].name()) {
				return i;
			}
		}
		assert(false);
		return -1;
	}
	else {
		return -1;
	}
}

int findParagraphStyle(ScribusDoc* doc, const QString &name)
{
	for (uint i=0; i < doc->paragraphStyles().count(); ++i)
	{
		if (name == doc->paragraphStyles()[i].name()) {
			return i;
		}
	}
	assert(false);
	return -1;
}

// FIXME: to be removed in full Qt4 port!
QPixmap getQCheckBoxPixmap(const bool checked, const QColor background)
{
	QCheckBox *tmpItem = new QCheckBox("", 0, "tmpItem");
	tmpItem->setMaximumSize(QSize(30, 30));
	tmpItem->setMinimumSize(QSize(30, 30));
	tmpItem->setPaletteBackgroundColor(background);
	tmpItem->setChecked(checked);
	QPixmap pm = QPixmap::grabWidget(tmpItem);
	pm.setMask(pm.createHeuristicMask());
	delete tmpItem;
	return pm;
}


void tDebug(QString message)
{
	QDateTime debugTime;
	qDebug("%s", QString("%1\t%2").arg(debugTime.currentDateTime().toString("hh:mm:ss:zzz")).arg(message).ascii());
}

QString getImageType(QString filename)
{
	QString ret = "";
	QFile f(filename);
	QFileInfo fi(f);
	if (fi.exists())
	{
		QByteArray buf(20);
		if (f.open(QIODevice::ReadOnly))
		{
			f.readBlock(buf.data(), 20);
			if ((buf[0] == '%') && (buf[1] == '!') && (buf[2] == 'P') && (buf[3] == 'S') && (buf[4] == '-') && (buf[5] == 'A'))
				ret = "eps";
			else if ((buf[0] == '\xC5') && (buf[1] == '\xD0') && (buf[2] == '\xD3') && (buf[3] == '\xC6'))
				ret = "eps";
			else if ((buf[0] == 'G') && (buf[1] == 'I') && (buf[2] == 'F') && (buf[3] == '8'))
				ret = "gif";
			else if ((buf[0] == '\xFF') && (buf[1] == '\xD8') && (buf[2] == '\xFF'))
				ret = "jpg";
			else if ((buf[0] == '%') && (buf[1] == 'P') && (buf[2] == 'D') && (buf[3] == 'F'))
				ret = "pdf";
			else if ((buf[0] == '\x89') && (buf[1] == 'P') && (buf[2] == 'N') && (buf[3] == 'G'))
				ret = "png";
			else if ((buf[0] == '8') && (buf[1] == 'B') && (buf[2] == 'P') && (buf[3] == 'S'))
				ret = "psd";
			else if (((buf[0] == 'I') && (buf[1] == 'I') && (buf[2] == '\x2A')) || ((buf[0] == 'M') && (buf[1] == 'M') && (buf[3] == '\x2A')))
				ret = "tif";
			else if ((buf[0] == '/') && (buf[1] == '*') && (buf[2] == ' ') && (buf[3] == 'X') && (buf[4] == 'P') && (buf[5] == 'M'))
				ret = "xpm";
			
			f.close();
		}
	}
	return ret;
}

QString readLinefromDataStream(QDataStream &s)
{
	QString ret = "";
	uchar charData;
	while (!s.atEnd())
	{
		s >> charData;
		if (charData == '\x0A')
			break;
		if (charData == '\x0D')
		{
			quint64 oldPos = s.device()->pos();
			s >> charData;
			if (charData != '\x0A')
				s.device()->seek(oldPos);
			break;
		}
		ret += QChar(charData);
	}
	return ret.trimmed();
}
