/*
 * Copyright (C) 2012 Cameron White
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

#include "fingerdialog.h"
#include "ui_fingerdialog.h"

#include <QMessageBox>

#include <score/note.h>

FingerDialog::FingerDialog(QWidget *parent, Note::FingerLeft fingerLeft)
: QDialog(parent),
ui(new Ui::FingerDialog)
{
    ui->setupUi(this);
    ui->fingerLeftComboBox->setCurrentIndex(fingerLeft);
}

FingerDialog::~FingerDialog()
{
    delete ui;
}

Note::FingerLeft FingerDialog::getFingerLeft() const
{
    return (Note::FingerLeft)ui->fingerLeftComboBox->currentIndex();
}

void FingerDialog::accept()
{
    done(Accepted);
}
