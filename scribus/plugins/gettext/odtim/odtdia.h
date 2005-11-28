 /***************************************************************************
  *   Copyright (C) 2004 by Riku Leino                                      *
  *   tsoots@gmail.com                                                      *
  *                                                                         *
  *   This program is free software; you can redistribute it and/or modify  *
  *   it under the terms of the GNU General Public License as published by  *
  *   the Free Software Foundation; either version 2 of the License, or     *
  *   (at your option) any later version.                                   *
  *                                                                         *
  *   This program is distributed in the hope that it will be useful,       *
  *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
  *   GNU General Public License for more details.                          *
  *                                                                         *
  *   You should have received a copy of the GNU General Public License     *
  *   along with this program; if not, write to the                         *
  *   Free Software Foundation, Inc.,                                       *
  *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef ODTDIA_H
#define ODTDIA_H

#include "config.h"

#ifdef HAVE_XML

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qstringlist.h>

class OdtDialog : public QDialog
{
	Q_OBJECT

public:
	OdtDialog(bool update, bool prefix, bool pack);
	~OdtDialog();
	bool shouldUpdate();
	bool usePrefix();
	bool askAgain();
	bool packStyles();
private:
	QCheckBox* updateCheck;
	QCheckBox* prefixCheck;
	QCheckBox* doNotAskCheck;
	QCheckBox* packCheck;
	QPushButton* okButton;
	QPushButton* cancelButton;
};

#endif // HAVE_XML

#endif // ODTDIA_H
