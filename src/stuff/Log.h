/*
 * Log.h
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#ifndef SRC_STUFF_LOG_H_
#define SRC_STUFF_LOG_H_

#include "../lib/base/base.h"
#include "../lib/pattern/Observable.h"

#ifdef ERROR
#undef ERROR
#endif

class Session;

class Log : public Observable<VirtualBase> {
public:
	Log();

	static const string MESSAGE_ADD;

	void error(Session *session, const string &message);
	void warn(Session *session, const string &message);
	void info(Session *session, const string &message);
	void debug(Session *session, const string &message);
	void question(Session *session, const string &message, const Array<string> &responses);

	enum class Type {
		ERROR,
		WARNING,
		QUESTION,
		INFO,
		DEBUG
	};

	struct Message {
		Session *session;
		Type type;
		string text;
		Array<string> responses;
		bool operator==(const Message &o) const;
	};

	Array<Message> all(Session *session);

	bool allow_debug;
	bool allow_console_output;

private:
	void add_message(Session *session, Type type, const string &message, const Array<string> &responses);
	Array<Message> messages;
	Array<Message> blocked;
};

#endif /* SRC_STUFF_LOG_H_ */
