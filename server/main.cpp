/*
 * Copyright (C) 2013 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pch.h"
#include <handlers.hpp>
#include <dbconn.hpp>
#include <locale.hpp>
#include <string.h>

#define THREAD_COUNT 1

#ifdef _WIN32
#define LOCALE_PATH "..\\locales\\"
#define LOG_FILE "..\\reedr.log"
#endif

#ifdef POSIX
#define LOCALE_PATH "../locales/"
#define LOG_FILE "../reedr.log"
#endif

REGISTER_REDIRECT("/", "/view/");

class Thread: public FastCGI::Thread
{
	unsigned long m_load;
public:
	Thread(): m_load(0) {}
	Thread(const char* uri):  FastCGI::Thread(uri) {}
	void onRequest(FastCGI::Request& req)
	{
		++m_load;
		FastCGI::app::HandlerPtr handler = FastCGI::app::Handlers::handler(req);
		if (handler.get() != nullptr)
			handler->visit(req);
		else
			req.on404();
	}
	unsigned long getLoad() const
	{
		return m_load;
	}
};

int main (int argc, char* argv[])
{
	FastCGI::FLogSource log(LOG_FILE);
	FLOG << "Application started";

	db::environment env;
	if (env.failed)
		return 1;

	FastCGI::Application app;

	int ret = app.init(LOCALE_PATH);
	if (ret != 0)
	{
		//FLOG << "app.init(" << LOCALE_PATH << ") failed: " << ret;
		return ret;
	}

	//FLOG << "app.init(" << LOCALE_PATH << ");";

	if (argc > 2 && !strcmp(argv[1], "-uri"))
	{
		Thread local(argv[2]);
		local.setApplication(app);

		app.addStlSession();

		local.handleRequest();
		return 0;
	}

	if (!app.addThreads<Thread>(THREAD_COUNT))
		return 1;

	app.run();
	return 0;
}
