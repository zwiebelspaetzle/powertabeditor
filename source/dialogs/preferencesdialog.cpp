/*
  * Copyright (C) 2011 Cameron White
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
  
#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include <QSettings>

#include <boost/foreach.hpp>

#include <audio/rtmidiwrapper.h>
#include <app/settings.h>
#include <app/pubsub/settingspubsub.h>
#include <app/skinmanager.h>
#include <dialogs/tuningdialog.h>
#include <powertabdocument/generalmidi.h>

typedef std::pair<int, int> MidiApiAndPort;
Q_DECLARE_METATYPE(MidiApiAndPort)

PreferencesDialog::PreferencesDialog(QWidget *parent,
                                     boost::shared_ptr<SettingsPubSub> pubsub,
                                     boost::shared_ptr<TuningDictionary> tuningDictionary) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog), pubsub(pubsub), tuningDictionary(tuningDictionary)
{
    ui->setupUi(this);

    connect(ui->useSkinCheckBox, SIGNAL(toggled(bool)),
            ui->skinComboBox, SLOT(setEnabled(bool)));

    // add available MIDI ports
    RtMidiWrapper rtMidiWrapper;
    for (size_t i = 0; i < rtMidiWrapper.getApiCount(); ++i)
    {
        for(uint32_t j = 0; j < rtMidiWrapper.getPortCount(i); j++)
        {
            std::string portName = rtMidiWrapper.getPortName(i, j);
            ui->midiPortComboBox->addItem(QString::fromStdString(portName),
                        QVariant::fromValue(std::pair<int, int>(i, j)));
        }
    }

    std::vector<std::string> presetNames;
    midi::GetMidiPresetNames(presetNames);
    BOOST_FOREACH(const std::string& name, presetNames)
    {
        ui->metronomePresetComboBox->addItem(QString::fromStdString(name));
    }

    ui->vibratoStrengthSpinBox->setRange(1, 127);
    ui->wideVibratoStrengthSpinBox->setRange(1, 127);

    ui->skinComboBox->addItems(SkinManager::availableSkins());

    BOOST_FOREACH(const std::string& name, presetNames)
    {
        ui->defaultPresetComboBox->addItem(QString::fromStdString(name));
    }

    loadCurrentSettings();
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

/// Load the current preferences and initialize the widgets with those values
void PreferencesDialog::loadCurrentSettings()
{
    QSettings settings;

    const int api = settings.value(Settings::MIDI_PREFERRED_API,
                              Settings::MIDI_PREFERRED_API_DEFAULT).toInt();
    const int port = settings.value(Settings::MIDI_PREFERRED_PORT,
                              Settings::MIDI_PREFERRED_PORT_DEFAULT).toInt();

    // Find the preferred midi port in the combo box.
    RtMidiWrapper rtMidiWrapper;
    ui->midiPortComboBox->setCurrentIndex(ui->midiPortComboBox->findText(
                QString::fromStdString(rtMidiWrapper.getPortName(api, port))));

    ui->metronomeEnabledCheckBox->setChecked(settings.value(Settings::MIDI_METRONOME_ENABLED,
                                                            Settings::MIDI_METRONOME_ENABLED_DEFAULT).toBool());

    ui->metronomePresetComboBox->setCurrentIndex(settings.value(
            Settings::MIDI_METRONOME_PRESET,
            Settings::MIDI_METRONOME_PRESET_DEFAULT).toInt());

    ui->vibratoStrengthSpinBox->setValue(settings.value(Settings::MIDI_VIBRATO_LEVEL,
                                                        Settings::MIDI_VIBRATO_LEVEL_DEFAULT).toUInt());

    ui->wideVibratoStrengthSpinBox->setValue(settings.value(Settings::MIDI_WIDE_VIBRATO_LEVEL,
                                                            Settings::MIDI_WIDE_VIBRATO_LEVEL_DEFAULT).toUInt());

    const int skinIndex = ui->skinComboBox->findText(settings.value(Settings::APPEARANCE_SKIN_NAME).toString());
    ui->skinComboBox->setCurrentIndex(skinIndex == -1 ? 0 : skinIndex);

    ui->useSkinCheckBox->setChecked(settings.value(Settings::APPEARANCE_USE_SKIN, false).toBool());
    ui->skinComboBox->setEnabled(ui->useSkinCheckBox->isChecked());

    ui->defaultInstrumentNameLineEdit->setText(settings.value(
            Settings::DEFAULT_INSTRUMENT_NAME,
            Settings::DEFAULT_INSTRUMENT_NAME_DEFAULT).toString());
    ui->defaultPresetComboBox->setCurrentIndex(settings.value(
            Settings::DEFAULT_INSTRUMENT_PRESET,
            Settings::DEFAULT_INSTRUMENT_PRESET_DEFAULT).toInt());

    ui->defaultTuningClickButton->setToolTip(tr("Click to adjust tuning."));
    defaultInstrumentTuning = settings.value(
            Settings::DEFAULT_INSTRUMENT_TUNING,
            QVariant::fromValue(Settings::DEFAULT_INSTRUMENT_TUNING_DEFAULT)).value<Tuning>();
    ui->defaultTuningClickButton->setText(QString::fromStdString(
            defaultInstrumentTuning.GetSpelling()));
    connect(ui->defaultTuningClickButton, SIGNAL(clicked()), this, SLOT(editTuning()));
}

/// Save the new settings
void PreferencesDialog::accept()
{
    QSettings settings;

    MidiApiAndPort apiAndPort = ui->midiPortComboBox->itemData(
                ui->midiPortComboBox->currentIndex()).value<MidiApiAndPort>();
    settings.setValue(Settings::MIDI_PREFERRED_API, apiAndPort.first);
    settings.setValue(Settings::MIDI_PREFERRED_PORT, apiAndPort.second);

    settings.setValue(Settings::MIDI_METRONOME_ENABLED,
                      ui->metronomeEnabledCheckBox->isChecked());

    settings.setValue(Settings::MIDI_METRONOME_PRESET,
                      ui->metronomePresetComboBox->currentIndex());

    settings.setValue(Settings::MIDI_VIBRATO_LEVEL,
                      ui->vibratoStrengthSpinBox->value());

    settings.setValue(Settings::MIDI_WIDE_VIBRATO_LEVEL,
                      ui->wideVibratoStrengthSpinBox->value());

    settings.setValue(Settings::APPEARANCE_USE_SKIN,
                      ui->useSkinCheckBox->isChecked());

    settings.setValue(Settings::APPEARANCE_SKIN_NAME,
                      ui->skinComboBox->currentText());

    settings.setValue(Settings::DEFAULT_INSTRUMENT_NAME,
                      ui->defaultInstrumentNameLineEdit->text());

    settings.setValue(Settings::DEFAULT_INSTRUMENT_PRESET,
                      ui->defaultPresetComboBox->currentIndex());

    settings.setValue(Settings::DEFAULT_INSTRUMENT_TUNING,
                      QVariant::fromValue(defaultInstrumentTuning));

    settings.sync();

    pubsub->publish(Settings::MIDI_METRONOME_ENABLED);

    done(Accepted);
}

void PreferencesDialog::editTuning()
{
    TuningDialog dialog(this, boost::shared_ptr<const Guitar>(),
                        defaultInstrumentTuning, tuningDictionary);

    if (dialog.exec() == QDialog::Accepted)
    {
        defaultInstrumentTuning = dialog.getNewTuning();
        ui->defaultTuningClickButton->setText(
                QString::fromStdString(defaultInstrumentTuning.GetSpelling()));
    }
}
