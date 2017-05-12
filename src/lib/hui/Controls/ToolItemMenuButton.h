/*
 * ToolItemMenuButton.h
 *
 *  Created on: 26.06.2013
 *      Author: michi
 */

#ifndef TOOLITEMMENUBUTTON_H_
#define TOOLITEMMENUBUTTON_H_

#include "Control.h"

namespace hui
{

class Menu;

class ToolItemMenuButton : public Control
{
public:
	ToolItemMenuButton(const string &title, Menu *menu, const string &image, const string &id);
};

}

#endif /* TOOLITEMMENUBUTTON_H_ */
