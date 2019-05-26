/*
 * ModuleConfiguration.h
 *
 *  Created on: 06.03.2019
 *      Author: michi
 */

#ifndef SRC_MODULE_MODULECONFIGURATION_H_
#define SRC_MODULE_MODULECONFIGURATION_H_


#include "../lib/base/base.h"

namespace Kaba{
	class Script;
	class Class;
};
class Session;


class ModuleConfiguration : public VirtualBase
{
public:
	ModuleConfiguration(){ _class = nullptr; }
	virtual ~ModuleConfiguration(){}
	void _cdecl __init__();
	virtual void _cdecl __delete__();
	virtual void _cdecl reset(){}
	virtual string to_string() const;
	virtual void from_string(const string &s, Session *session);
	const Kaba::Class *_class;
};

string var_to_string(const Kaba::Class *c, char *v);
void var_from_string(const Kaba::Class *type, char *v, const string &s, int &pos, Session *session);


#endif /* SRC_MODULE_MODULECONFIGURATION_H_ */
