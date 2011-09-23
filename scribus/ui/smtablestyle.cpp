/*
Copyright (C) 2011 Elvis Stansvik <elvstone@gmail.com>

For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include <QTabWidget>

#include "prefsmanager.h"
#include "smtablestyle.h"
#include "smtablestylewidget.h"

SMTableStyle::SMTableStyle() : StyleItem(),m_widget(0), m_page(0), m_doc(0)
{
	m_widget = new QTabWidget();
	Q_CHECK_PTR(m_widget);
	m_widget->setContentsMargins(5, 5, 5, 5);

	m_page = new SMTableStyleWidget();
	Q_CHECK_PTR(m_page);
	
	m_widget->addTab(m_page, tr("Properties"));
}

SMTableStyle::~SMTableStyle()
{
	delete m_page;
	delete m_widget;
	m_page = 0;
	m_widget = 0;
}

QTabWidget* SMTableStyle::widget()
{
	return m_widget;
}

QString SMTableStyle::typeNamePlural()
{
	return tr("Table Styles");
}

QString SMTableStyle::typeNameSingular()
{
	return tr("Table Style");
}

void SMTableStyle::setCurrentDoc(ScribusDoc *doc)
{
	m_doc = doc;

	if (!m_doc)
	{
		removeConnections();
		m_selection.clear();
		m_cachedStyles.clear();
	}

	if (m_doc)
	{
		if (m_page)
		{
			m_page->fillFillColorCombo(m_doc->PageColors);
		}
	}
	else
	{
		removeConnections();
		m_selection.clear();
		m_cachedStyles.clear();
	}
}

QList<StyleName> SMTableStyle::styles(bool reloadFromDoc)
{
	QList<StyleName> stylesList;

	if (!m_doc)
		return stylesList; // No document available.

	if (reloadFromDoc)
		updateStylesCache(); // Update cache.

	// Return a list of names of cached styles.
	for (int i = 0; i < m_cachedStyles.count(); ++i)
	{
		if (m_cachedStyles[i].hasName())
		{
			QString styleName(m_cachedStyles[i].displayName());
			QString parentName(QString::null);

			if (m_cachedStyles[i].hasParent())
			{
				const Style* parentStyle = m_cachedStyles[i].parentStyle();
				if (parentStyle)
					parentName = parentStyle->displayName();
			}

			stylesList << StyleName(styleName, parentName);
		}
	}

	return stylesList;
}

void SMTableStyle::reload()
{
	updateStylesCache();
}

void SMTableStyle::selected(const QStringList &styleNames)
{
	m_selection.clear();
	m_selectionIsDirty = false;
	removeConnections();
	QList<TableStyle> tableStyles;

	m_cachedStyles.invalidate();

	for (int i = 0; i < m_cachedStyles.count(); ++i)
		tableStyles << m_cachedStyles[i];

	foreach (QString styleName, styleNames)
	{
		int index = m_cachedStyles.find(styleName);
		// FIXME: #7133: Use .isDefaultStyle() instead here rather than relying on tr text comparison
		if (index < 0 && styleName == CommonStrings::trDefaultTableStyle)
			index = m_cachedStyles.find(CommonStrings::DefaultTableStyle);
		if (index > -1)
			m_selection.append(&m_cachedStyles[index]);
	}
	m_page->show(m_selection, tableStyles, PrefsManager::instance()->appPrefs.hyphPrefs.Language, m_doc->unitIndex());
	setupConnections();
}

QString SMTableStyle::fromSelection() const
{
	// TODO: Implement this once we have table items.
	return QString::null;
}

void SMTableStyle::toSelection(const QString &styleName) const
{
	// TODO: Implement this once we have table items.
}

QString SMTableStyle::newStyle()
{
	Q_ASSERT(m_doc);

	QString name = getUniqueName(tr("New Style"));
	TableStyle style;
	style.setDefaultStyle(false);
	style.setName(name);
	m_cachedStyles.create(style);
	return name;
}

QString SMTableStyle::newStyle(const QString &fromStyle)
{
	// #7179, do our name switch yet again to handle this properly for default styles
	// FIXME: use isDefaultStyle somehow
	QString fromStyleName(fromStyle);
	if (fromStyle == CommonStrings::trDefaultTableStyle)
		fromStyleName = CommonStrings::DefaultTableStyle;

	Q_ASSERT(m_cachedStyles.resolve(fromStyleName));
	if (!m_cachedStyles.resolve(fromStyleName))
		return QString::null;

	// Copy the style with name constructed from the original name.
	QString styleName(getUniqueName(tr("Clone of %1").arg(fromStyle)));
	TableStyle tableStyle(m_cachedStyles.get(fromStyleName));
	tableStyle.setDefaultStyle(false);
	tableStyle.setName(styleName);
	tableStyle.setShortcut(QString::null);
	m_cachedStyles.create(tableStyle);

	return styleName;
}

void SMTableStyle::apply()
{
	if (!m_doc)
		return; // No document available.

	// Handle replacement of deleted styles.
	QMap<QString, QString> replacement;
	for (int i = 0; i < m_deleted.count(); ++i)
	{
		if (m_deleted[i].first == m_deleted[i].second)
			continue; // Nothing to do.
		replacement[m_deleted[i].first] = m_deleted[i].second;
	}

	m_doc->redefineTableStyles(m_cachedStyles, false);
	m_doc->replaceTableStyles(replacement);

	m_deleted.clear(); // Deletion done at this point.

	// TODO: We should probably have something similar to this for tables/cells.
	//m_doc->scMW()->requestUpdate(reqTextStylesUpdate);

	m_doc->changed();
}

void SMTableStyle::editMode(bool isOn)
{
	if (isOn)
		updateStylesCache();
}

bool SMTableStyle::isDefaultStyle(const QString &styleName) const
{
	int index = m_cachedStyles.find(styleName);
	bool result = false;
	if (index > -1)
		result = m_cachedStyles[index].isDefaultStyle();
	else
	{
		if (CommonStrings::trDefaultTableStyle == styleName)
		{
			index = m_cachedStyles.find(CommonStrings::DefaultTableStyle);
			if (index > -1)
				result = m_cachedStyles[index].isDefaultStyle();
		}
	}
	return result;
}

void SMTableStyle::setDefaultStyle(bool isDefaultStyle)
{
	Q_ASSERT(m_selection.count() == 1);
	if (m_selection.count() != 1)
		return;

	m_selection[0]->setDefaultStyle(isDefaultStyle);
	
	if (!m_selectionIsDirty)
	{
		m_selectionIsDirty = true;
		emit selectionDirty();
	}
}

QString SMTableStyle::shortcut(const QString &styleName) const
{
	QString result(QString::null);
	int index = m_cachedStyles.find(styleName);
	if (index > -1)
		result = m_cachedStyles[index].shortcut();
	else
	{
		// FIXME: Use isDefaultStyle somehow.
		if (CommonStrings::trDefaultTableStyle == styleName)
		{
			index = m_cachedStyles.find(CommonStrings::DefaultTableStyle);
			if (index > -1)
				result = m_cachedStyles[index].shortcut();
		}
	}
	return result;
}

void SMTableStyle::setShortcut(const QString &shortcut)
{
	Q_ASSERT(m_selection.count() == 1);
	if (m_selection.count() != 1)
		return;

	m_selection[0]->setShortcut(shortcut);

	if (!m_selectionIsDirty)
	{
		m_selectionIsDirty = true;
		emit selectionDirty();
	}
}

void SMTableStyle::deleteStyles(const QList<RemoveItem> &removeList)
{
	foreach (RemoveItem removeItem, removeList)
	{
		for (int i = 0; i < m_selection.count(); ++i)
		{
			if (m_selection[i]->name() == removeItem.first)
			{
				m_selection.removeAt(i);
				break;
			}
		}

		int index = m_cachedStyles.find(removeItem.first);
		if (index > -1)
			m_cachedStyles.remove(index);
		m_deleted << removeItem;
	}
}

void SMTableStyle::nameChanged(const QString &newName)
{
	// Save the old name.
	QString oldName(m_selection[0]->name());

	// Make a copy of the old style but with new name.
	TableStyle newStyle(*m_selection[0]);
	newStyle.setName(newName);
	m_cachedStyles.create(newStyle);

	// Select the new style.
	m_selection.clear();
	m_selection.append(&m_cachedStyles[m_cachedStyles.find(newName)]);

	// Remove old style from cache.
	for (int j = 0; j < m_cachedStyles.count(); ++j)
	{
		int index = m_cachedStyles.find(oldName);
		if (index > -1)
		{
			m_cachedStyles.remove(index);
			break;
		}
	}

	// Set parent references to old style to new style.
	for (int j = 0; j < m_cachedStyles.count(); ++j)
	{
		if (m_cachedStyles[j].parent() == oldName)
			m_cachedStyles[j].setParent(newName);
	}

	// Update the deleted list to reflect the name change.
	QList<RemoveItem>::iterator it;
	for (it = m_deleted.begin(); it != m_deleted.end(); ++it)
	{
		if ((*it).second == oldName)
		{
			oldName = (*it).first;
			m_deleted.erase(it);
			break;
		}
	}
	m_deleted.append(RemoveItem(oldName, newName));

	// Mark selection as dirty.
	if (!m_selectionIsDirty)
	{
		m_selectionIsDirty = true;
		emit selectionDirty();
	}
}

QString SMTableStyle::getUniqueName(const QString &name)
{
	int id = 0;
	bool done = false;
	QString s(name);

	while (!done)
	{
start:
		++id;
		for (int i = 0; i < m_cachedStyles.count(); ++i)
		{
			if (m_cachedStyles[i].name() == s)
			{
				s = tr("%1 (%2)", "This for unique name when creating "
						"a new character style. %1 will be the name "
								"of the style and %2 will be a number forming "
								"a style name like: New Style (2)").arg(name).arg(id);
				goto start;
			}
		}
		done = true;
	}

	return s;
}

void SMTableStyle::languageChange()
{
	if (m_widget && m_page)
	{
		m_widget->setTabText(m_widget->indexOf(m_page), tr("Properties"));
		m_page->languageChange();
	}
}

void SMTableStyle::unitChange()
{
	// Unimplemented.
}

void SMTableStyle::updateStylesCache()
{
	if (!m_doc)
		return; // No document available.

	m_selection.clear();
	m_cachedStyles.clear();
	m_deleted.clear();

	m_cachedStyles.redefine(m_doc->tableStyles(), true);
}

void SMTableStyle::setupConnections()
{
	if (!m_page)
		return;

	connect(m_page->fillColor, SIGNAL(activated(const QString&)),
			this, SLOT(slotFillColor()));
}

void SMTableStyle::removeConnections()
{
	if (!m_page)
		return;

	disconnect(m_page->fillColor, SIGNAL(activated(const QString&)),
			this, SLOT(slotFillColor()));
}

void SMTableStyle::slotFillColor()
{
	// TODO: Handle inheritance as well.
	QString fillColor(m_page->fillColor->currentText());
	foreach (TableStyle *tableStyle, m_selection) {
		tableStyle->setFillColor(fillColor);
	}

	if (!m_selectionIsDirty)
	{
		m_selectionIsDirty = true;
		emit selectionDirty();
	}
}
