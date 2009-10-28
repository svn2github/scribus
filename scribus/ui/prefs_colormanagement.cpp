/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "prefs_colormanagement.h"
#include "scribusstructs.h"
#include "prefsstructs.h"

Prefs_ColorManagement::Prefs_ColorManagement(QWidget* parent)
	: Prefs_Pane(parent)
{
	setupUi(this);
	languageChange();
}

Prefs_ColorManagement::~Prefs_ColorManagement()
{
}

void Prefs_ColorManagement::languageChange()
{
}

void Prefs_ColorManagement::restoreDefaults(struct ApplicationPrefs *prefsData)
{
	activateCMCheckBox->setChecked(prefsData->colorPrefs.DCMSset.CMSinUse);

	QString tmp_mp[] = { tr("Perceptual"), tr("Relative Colorimetric"),
		tr("Saturation"), tr("Absolute Colorimetric")};
		size_t array = sizeof(tmp_mp) / sizeof(*tmp_mp);
	imageRenderingIntentComboBox->clear();
	for (uint prop = 0; prop < array; ++prop)
		imageRenderingIntentComboBox->addItem(tmp_mp[prop]);
	imageRenderingIntentComboBox->setCurrentIndex(prefsData->colorPrefs.DCMSset.DefaultIntentImages);
	solidColorsRenderingIntentComboBox->clear();
	for (uint prop = 0; prop < array; ++prop)
		solidColorsRenderingIntentComboBox->addItem(tmp_mp[prop]);
	solidColorsRenderingIntentComboBox->setCurrentIndex(prefsData->colorPrefs.DCMSset.DefaultIntentColors);

	simulatePrinterOnScreenCheckBox->setChecked(prefsData->colorPrefs.DCMSset.SoftProofOn);
	convertAllColorsToPrinterSpaceCheckBox->setChecked(prefsData->colorPrefs.DCMSset.SoftProofFullOn);
	markColorsOutOfGamutCheckBox->setChecked(prefsData->colorPrefs.DCMSset.GamutCheck);
	useBlackpointCompensationCheckBox->setChecked(prefsData->colorPrefs.DCMSset.BlackPoint);

	simulatePrinterOnScreenCheckBox->setEnabled( activateCMCheckBox->isChecked() );
	useBlackpointCompensationCheckBox->setEnabled( activateCMCheckBox->isChecked() );

}

void Prefs_ColorManagement::setProfiles(struct ApplicationPrefs *prefsData, ProfilesL *inputProfiles, ProfilesL *inputProfilesCMYK, ProfilesL *printerProfiles, ProfilesL *monitorProfiles)
{
	ProfilesL::Iterator it;
	rgbImageProfileComboBox->clear();
	for (it = inputProfiles->begin(); it != inputProfiles->end(); ++it)
	{
		rgbImageProfileComboBox->addItem(it.key());
		if (it.key() == prefsData->colorPrefs.DCMSset.DefaultImageRGBProfile)
			rgbImageProfileComboBox->setCurrentIndex(rgbImageProfileComboBox->count()-1);
	}
	cmykImageProfileComboBox->clear();
	for (it = inputProfilesCMYK->begin(); it != inputProfilesCMYK->end(); ++it)
	{
		cmykImageProfileComboBox->addItem(it.key());
		if (it.key() == prefsData->colorPrefs.DCMSset.DefaultImageCMYKProfile)
			cmykImageProfileComboBox->setCurrentIndex(cmykImageProfileComboBox->count()-1);
	}
	rgbSolidProfileComboBox->clear();
	for (it = inputProfiles->begin(); it != inputProfiles->end(); ++it)
	{
		rgbSolidProfileComboBox->addItem(it.key());
		if (it.key() == prefsData->colorPrefs.DCMSset.DefaultSolidColorRGBProfile)
			rgbSolidProfileComboBox->setCurrentIndex(rgbSolidProfileComboBox->count()-1);
	}
	cmykSolidProfileComboBox->clear();
	for (it = inputProfilesCMYK->begin(); it != inputProfilesCMYK->end(); ++it)
	{
		cmykSolidProfileComboBox->addItem(it.key());
		if (it.key() == prefsData->colorPrefs.DCMSset.DefaultSolidColorCMYKProfile)
			cmykSolidProfileComboBox->setCurrentIndex(cmykSolidProfileComboBox->count()-1);
	}
	monitorProfileComboBox->clear();
	for (it = monitorProfiles->begin(); it != monitorProfiles->end(); ++it)
	{
		monitorProfileComboBox->addItem(it.key());
		if (it.key() == prefsData->colorPrefs.DCMSset.DefaultMonitorProfile)
			monitorProfileComboBox->setCurrentIndex(monitorProfileComboBox->count()-1);
	}
	printerProfileComboBox->clear();
	for (it = printerProfiles->begin(); it != printerProfiles->end(); ++it)
	{
		printerProfileComboBox->addItem(it.key());
		if (it.key() == prefsData->colorPrefs.DCMSset.DefaultPrinterProfile)
			printerProfileComboBox->setCurrentIndex(printerProfileComboBox->count()-1);
	}
}

void Prefs_ColorManagement::saveGuiToPrefs(struct ApplicationPrefs *prefsData) const
{
}

