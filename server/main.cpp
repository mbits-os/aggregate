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

#ifdef _WIN32
#define LOCALE_PATH "..\\locales\\"
#endif

#ifdef POSIX
#define LOCALE_PATH "../locales/"
#endif

REGISTER_REDIRECT("/", "/view/");

void onRequest(FastCGI::Application& app)
{
	FastCGI::Request req(app);

	try {
		FastCGI::app::HandlerPtr handler = FastCGI::app::Handlers::handler(req);
		if (handler.get() != nullptr)
			handler->visit(req);
		else
			req.on404();

	} catch(FastCGI::FinishResponse) {
		// die() lands here
	}
}

int main (int argc, char* argv[])
{
	db::environment env;
	if (env.failed)
		return 1;

	if (argc > 2 && !strcmp(argv[1], "-uri"))
	{
		FastCGI::Application app(argv[2]);
		int ret = app.init(LOCALE_PATH);
		if (ret != 0)
			return ret;

		app.addStlSession();

		onRequest(app);
		return 0;
	}

	FastCGI::Application app;

	int ret = app.init(LOCALE_PATH);
	if (ret != 0)
		return ret;

	while (app.accept())
		onRequest(app);

	return 0;
}
