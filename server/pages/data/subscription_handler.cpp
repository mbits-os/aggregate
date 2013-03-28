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
#include <utils.hpp>

namespace FastCGI { namespace app { namespace reader {

	struct FolderInfo {
		const char* m_title;
		const char* m_id;
		unsigned long long m_unread;
		const char* m_parent;
	};

	struct FeedInfo {
		const char* m_title;
		const char* m_url;
		const char* m_feed;
		unsigned long long m_unread;
		const char* m_folder;
	};

	namespace {
		//hardcoded data for now...
		FolderInfo folders[] = {
			{ "fun", "fun", 1 },
			{ "tech", "tech", 2 },
			{ "news", "news", 0 },
		};
		FeedInfo feeds[] = {
			{ "Niebezpiecznik.pl", "http://niebezpiecznik.pl/", "http://feeds.feedburner.com/niebezpiecznik/", 2, "tech" },
			{ "Android Developers Blog", "http://android-developers.blogspot.com/", "http://feeds.feedburner.com/blogspot/hsDu", 0, "tech" },
			{ "Saturday Morning Breakfast Cereal (updated daily)", "http://www.smbc-comics.com", "http://feeds.feedburner.com/smbc-comics/PvLb", 0, "fun" },
			{ "TEDTalks (video)", "http://www.ted.com/talks/list", "http://feeds.feedburner.com/tedtalks_video", 0, "fun" },
			{ "The Big Picture", "http://www.boston.com/bigpicture/", "http://feeds.boston.com/boston/bigpicture/index", 1, "fun" },
			{ "The Daily WTF", "http://thedailywtf.com/", "http://syndication.thedailywtf.com/TheDailyWtf", 0, "fun" },
			{ "Has the Large Hadron Collider destroyed the world yet?", "http://www.hasthelargehadroncolliderdestroyedtheworldyet.com/", "http://www.hasthelargehadroncolliderdestroyedtheworldyet.com/atom.xml", 0, "news" },
			{ "xkcd.com", "http://xkcd.com/", "http://xkcd.com/atom.xml", 0 }
		};
		const char* user_id = "07279206485321865861";
	}

	class SubscriptionPageHandler: public Handler
	{
		std::string m_service_url;
	public:

		std::string name() const
		{
			return "JSON::Feeds";
		}

		static void folder(FastCGI::Request& request, const FolderInfo& nfo, bool add_comma) {
			if (add_comma)
				request << ",";
			request << 
				"{\"title\":\"" << nfo.m_title << "\", "
				"\"url\": \"user/" << user_id << "/" << nfo.m_id << "\", "
				"\"unread\": " << nfo.m_unread << ", ";
			if (nfo.m_parent && *nfo.m_parent)
				request << "\"parent\": \"" << nfo.m_parent << "\"}";
			else
				request << "\"parent\": null}";
		}

		static void feed(FastCGI::Request& request, const FeedInfo& nfo, bool add_comma) {
			if (add_comma)
				request << ",";
			request << 
				"{\"title\":\"" << nfo.m_title << "\", "
				"\"url\": \"" << nfo.m_url << "\", "
				"\"feed_url\": \"" << nfo.m_feed << "\", "
				"\"unread\": " << nfo.m_unread << ", ";
			if (nfo.m_folder && *nfo.m_folder)
				request << "\"folder\": \"" << nfo.m_folder << "\"}";
			else
				request << "\"folder\": null}";
		}

		void visit(FastCGI::Request& request)
		{
			request.setHeader("Content-Type", "application/json; charset=utf-8");
			request << "{\"folders\":[";
			for (size_t i = 0; i < array_size(folders); ++i)
				folder(request, folders[i], i != 0);
			request << "], \"feeds\":[";
			for (size_t i = 0; i < array_size(feeds); ++i)
				feed(request, feeds[i], i != 0);
			request << "]}";
		}

	};
}}}  // FastCGI::app::reader

REGISTER_HANDLER("/data/subscription.json", FastCGI::app::reader::SubscriptionPageHandler);
