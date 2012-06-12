/*
 * Copyright (C) 2011 Jain Basil Aliyas <jainbasil@gmail.com>
 *
 * For general Scribus (>=1.3.2) copyright and licensing information please refer
 * to the COPYING file provided with the program. Following this notice may exist
 * a copyright and/or license notice that predates the release of Scribus 1.3.2
 * for which a new license (GPL+exception) is in place.
 */

#include "api_color.h"
#include "sccolorengine.h"
#include "scribusdoc.h"
#include "scribuscore.h"
#include "commonstrings.h"
#include "sccolor.h"
#include "utils.h"

ScColorWrapper::ScColorWrapper(ScColor *l, QString n) : QObject(COLLECTOR)
{
    qDebug() << "ScColorWrapper loaded";
    setObjectName("scColorWrapper");
	color = l;
	colorName = n;
}

QString ScColorWrapper::name()
{
	return colorName;
}

void ScColorWrapper::setName(QString name)
{
	colorName = name;
}

bool ScColorWrapper::isSpotColor()
{
	return color->isSpotColor();
}

void ScColorWrapper::setSpotColor(bool value)
{
	color->setSpotColor(value);
}

void ScColorWrapper::replace(QString replace)
{
	if(replace.isEmpty())
	{
		RAISE("Cannot replace a color with an empty name.");
	}
	if (ScCore->primaryMainWindow()->doc->PageColors.contains(colorName) 
		&& (ScCore->primaryMainWindow()->doc->PageColors.contains(replace) 
			|| (replace == CommonStrings::None)))
		ReplaceColor(colorName, replace);
	else
	{
		RAISE("Color not found.");
	}
}

void ScColorWrapper::remove(QString replace)
{
	if (ScCore->primaryMainWindow()->HaveDoc)
	{
		if (ScCore->primaryMainWindow()->doc->PageColors.contains(colorName) 
			&& (ScCore->primaryMainWindow()->doc->PageColors.contains(replace) 
				|| (replace == CommonStrings::None)))
		{
			ScCore->primaryMainWindow()->doc->PageColors.remove(colorName);
			ReplaceColor(colorName, replace);
		}
		else
		{
			RAISE("Color not found in document.");
			return;
		}
	}
	else
	{
		ColorList* colorList=PrefsManager::instance()->colorSetPtr();
		if (colorList->contains(colorName))
			colorList->remove(colorName);
		else
		{
			RAISE("Color not found in default colors");
			return;
		}
	}	
}

void ScColorWrapper::changeCMYK(int c, int m, int y, int k)
{
    if(ScCore->primaryMainWindow()->HaveDoc)
    {
        if(!ScCore->primaryMainWindow()->doc->PageColors.contains(colorName))
        {
            RAISE("Color not found in document");
        }
        ScCore->primaryMainWindow()->doc->PageColors[colorName].setColor(c, m, y, k);
    }
    else
    {
        ColorList* colorList=PrefsManager::instance()->colorSetPtr();
        if(!colorList->contains(colorName))
        {
            RAISE("Color not found in default colors.");
        }
        (*colorList)[colorName].setColor(c, m, y, k);
    }
}

void ScColorWrapper::changeRGB(int r, int g, int b)
{
	if(ScCore->primaryMainWindow()->HaveDoc)
	{
		if(!ScCore->primaryMainWindow()->doc->PageColors.contains(colorName))
		{
			RAISE("Color not found in document");
		}
		ScCore->primaryMainWindow()->doc->PageColors[colorName].setColorRGB(r, g, b);
	}
	else 
	{
		ColorList* colorList=PrefsManager::instance()->colorSetPtr();
		if(!colorList->contains(colorName))
		{
			RAISE("Color not found in default colors");
		}
		(*colorList)[colorName].setColorRGB(r, g, b);
	}
}

ScColorWrapper::~ScColorWrapper()
{
	qDebug() << "ScColorWrapper deleted";
}