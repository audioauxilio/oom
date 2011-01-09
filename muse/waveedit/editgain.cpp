//
// C++ Implementation: editgain
//
// Description:
//
//
// Author: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <QDialog>

#include "editgain.h"

EditGain::EditGain(QWidget* parent, int initGainValue)
 : QDialog(parent)
      {
         setupUi(this);
         sliderGain->setValue(sliderGain->maximum() - initGainValue);
         connect(buttonReset, SIGNAL(pressed()), this, SLOT(resetPressed()));
         connect(buttonApply, SIGNAL(pressed()), this, SLOT(applyPressed()));
         connect(buttonCancel,SIGNAL(pressed()), this, SLOT(cancelPressed()));
         connect(sliderGain,  SIGNAL(valueChanged(int)), this, SLOT(gainChanged(int)));
         if (sliderGain->value() != 100)
               buttonReset->setEnabled(true);
      }


EditGain::~EditGain()
      {
      }


/*!
    \fn EditGain::resetPressed
 */
void EditGain::resetPressed()
      {
      sliderGain->blockSignals(true);
      sliderGain->setValue(100);
      sliderGain->blockSignals(false);
      buttonReset->setEnabled(false);
      buttonApply->setEnabled(false);
      }


/*!
    \fn EditGain::applyPressed()
 */
void EditGain::applyPressed()
      {
      done(QDialog::Accepted);
      }


/*!
    \fn EditGain::cancelPressed()
 */
void EditGain::cancelPressed()
      {
      done(QDialog::Rejected);
      }



/*!
    \fn EditGain::gainChanged(int value)
 */
void EditGain::gainChanged(int value)
      {
      gain = sliderGain->maximum() - value;
      if (sliderGain->value() != 100) {
            buttonReset->setEnabled(true);
            buttonApply->setEnabled(true);
            }
      else {
            buttonReset->setEnabled(false);
            buttonApply->setEnabled(false);
            }
      }


/*!
    \fn EditGain::getGain()
 */
int EditGain::getGain()
      {
      return gain;
      }
