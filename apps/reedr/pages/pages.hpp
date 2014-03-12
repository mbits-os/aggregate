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

#ifndef __PAGES_HPP__
#define __PAGES_HPP__

#include <fast_cgi.hpp>
#include <top_menu.hpp>

namespace FastCGI { namespace app
{
	class PageHandler: public Handler
	{
	protected:
		virtual bool restrictedPage() { return true; }
		virtual const char* getPageTitle(Request& request, PageTranslation& tr)
		{
			ContentPtr content = request.getContent();
			if (content.get())
				return content->getPageTitle(tr);
			return nullptr;
		}
		std::string getTitle(Request& request, PageTranslation& tr)
		{
			std::string title = tr(lng::LNG_GLOBAL_SITENAME);
			const char* page = getPageTitle(request, tr);
			if (!page) return title;
			title += " &raquo; ";
			title += page;
			return title;
		} 
		//rendering the page
		virtual void prerender(const SessionPtr& session, Request& request, PageTranslation& tr) {}
		virtual void title(const SessionPtr& session, Request& request, PageTranslation& tr);
		virtual void header(const SessionPtr& session, Request& request, PageTranslation& tr);
		virtual void headElement(const SessionPtr& session, Request& request, PageTranslation& tr);
		virtual void buildTopMenu(TopMenu::TopBar& menu, const SessionPtr& session, Request& request, PageTranslation& tr);
		virtual void bodyStart(const SessionPtr& session, Request& request, PageTranslation& tr);
		virtual void render(const SessionPtr& session, Request& request, PageTranslation& tr)
		{
			ContentPtr content = request.getContent();
			if (content.get())
				content->render(session, request, tr);
		}
		virtual void footer(const SessionPtr& session, Request& request, PageTranslation& tr);
		virtual void bodyEnd(const SessionPtr& session, Request& request, PageTranslation& tr);
		virtual void topbarUI(const SessionPtr& session, Request& request, PageTranslation& tr);
		virtual void postrender(const SessionPtr& session, Request& request, PageTranslation& tr) {}
	public:
		void visit(Request& request);
	};
}} //FastCGI::app

#endif // __PAGES_HPP__