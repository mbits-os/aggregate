/*
 *  A simple FastCGI application example in C++.
 *
 *  $Id: echo-cpp.cpp,v 1.10 2002/02/25 00:46:17 robs Exp $
 *
 *  Copyright (c) 2001  Rob Saccoccio and Chelsea Networks
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
extern char ** environ;
#endif
#include "fast_cgi.h"

using namespace std;

// Maximum number of bytes allowed to be read from stdin
static const unsigned long STDIN_MAX = 1000000;

static void penv(const char * const * envp)
{
    cout << "<PRE>\n";
    for ( ; *envp; ++envp)
    {
        cout << *envp << "\n";
    }
    cout << "</PRE>\n";
}

int main (void)
{
	FastCGI::Application app;

	int count = 0;

	int ret = app.init();
	if (ret != 0)
		return ret;

	while (app.accept())
    {
		FastCGI::Request req(app);

        // Although FastCGI supports writing before reading,
        // many http clients (browsers) don't support it (so
        // the connection deadlocks until a timeout expires!).
		req.readContents(true);

        cout << "Content-type: text/html\r\n"
                "\r\n"
                "<TITLE>echo-cpp</TITLE>\n"
                "<H1>echo-cpp</H1>\n"
                "<H4>PID: " << app.pid() << "</H4>\n"
                "<H4>Request Number: " << ++count << "</H4>\n";

        cout << "<H4>Request Environment</H4>\n";
        penv(req.envp());

        cout << "<H4>Process/Initial Environment</H4>\n";
        penv(environ);

		cout << "<H4>Standard Input - " << req.size();
        if (req.size() == req.stdin_max) cout << " (STDIN_MAX)";
        cout << " bytes</H4>\n";
        if (req.size() && req.contents()) cout.write(req.contents(), req.size());
    }

    return 0;
}
