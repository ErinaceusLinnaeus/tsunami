/*
 * RecordDialog.cpp
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#include "RecordDialog.h"

RecordDialog::RecordDialog(hui::Window *_parent, bool _allow_parent):
	hui::Window("dummy", -1, -1, 800, 600, _parent, _allow_parent, 0)
{
}

RecordDialog::~RecordDialog()
{
}
