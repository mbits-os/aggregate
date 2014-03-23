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

#include <iostream>
#include <map>

#include <db/conn.hpp>
#include <utils.hpp>

#include <http/http.hpp>
#include <feed_parser.hpp>
#include <opml.hpp>

#include <sstream>

#ifdef WIN32
#include <windows.h>
#endif

#include <uri.hpp>

std::string stripws(const std::string& n)
{
	std::string out;
	out.reserve(n.length());
	bool was_space = true; //for lstrip
	for (auto c = n.begin(); c != n.end(); ++c)
	{
		if (isspace((unsigned char)*c))
		{
			if (!was_space)
			{
				was_space = true;
				out.push_back(' ');
			}
		}
		else
		{
			was_space = false;
			out.push_back(*c);
		}
	}
	if (was_space)
		out = out.substr(0, out.length()-1);
	return out;
}

class OnReadyStateChange: public http::XmlHttpRequest::OnReadyStateChange
{
	size_t m_read;
	void onReadyStateChange(http::XmlHttpRequest* xhr)
	{
		auto state = xhr->getReadyState();
		auto text = xhr->getResponseText();
		auto len = xhr->getResponseTextLength();
		auto block = len - m_read;
		text += m_read;
		m_read = len;
		printf("....calback: %d %d (%s); %p + %u\n", state, xhr->getStatus(), xhr->getStatusText().c_str(), text, block);

		if (state == http::XmlHttpRequest::HEADERS_RECEIVED)
		{
			if (xhr->wasRedirected())
				printf("REDIRECTED TO: %s\n", xhr->getFinalLocation().c_str());

			printf("HEADERS:\n");
			auto headers = xhr->getResponseHeaders();
			for (auto&& value: headers)
			{
				bool upcase = true;
				std::string head = value.first;
				std::transform(head.begin(), head.end(), head.begin(), [&upcase](char c) -> char
				{
					if (upcase)
					{
						upcase = false;
						return toupper(c);
					}
					if (c == '-')
						upcase = true;
					return c;
				});
				printf("%s: %s\n", head.c_str(), value.second.c_str());
			};
			printf("\n");
		}

		if (state == http::XmlHttpRequest::DONE && (xhr->getStatus() / 100 == 2))
		{
#ifdef WIN32
			auto win32_cp = GetConsoleCP();
			SetConsoleCP(CP_UTF8);
#endif
			printf("CONTENTS:\n---------\n");
			auto xml = xhr->getResponseXml();
			if (xml)
			{
				feed::Feed feed;
				if (feed::parse(xml, feed))
				{
					if (!feed.m_feed.m_title.empty()) std::cout << "\"" << stripws(feed.m_feed.m_title) << "\"" << std::endl;
					else std::cout << "(no title)" << std::endl;
					if (!feed.m_feed.m_url.empty()) std::cout << "href: " << feed.m_feed.m_url << std::endl;
					if (!feed.m_description.empty()) printf("%s\n", feed.m_description.c_str());
					if (!feed.m_author.m_name.empty() || !feed.m_author.m_email.empty())
					{
						std::cout << "by " << feed.m_author.m_name;
						if (!feed.m_author.m_email.empty())
							std::cout << " <" << feed.m_author.m_email << ">" << std::endl;
					}
					if (feed.m_categories.size() > 0)
					{
						std::cout << "categories:";
						bool first = true;
						for (auto& category: feed.m_categories)
						{
							if (first) first = false;
							else std::cout << ",";
							std::cout << " " << category;
						};
						std::cout << std::endl;
					}

					for (auto& entry: feed.m_entry)
					{
						std::cout << "    ------------------------------------------------------------\n\n";
						if (!entry.m_entry.m_title.empty()) std::cout << "    \"" << stripws(entry.m_entry.m_title) << "\"" << std::endl;
						else std::cout << "    (no title)" << std::endl;
						if (!entry.m_entry.m_url.empty()) std::cout << "    href: " << entry.m_entry.m_url << std::endl;
						if (!entry.m_entryUniqueId.empty()) std::cout << "    guid: " << entry.m_entryUniqueId << std::endl;
						else std::cout << "    guid missing!" << std::endl;
						if (entry.m_dateTime != -1)
						{
							char timebuf[100];
							tyme::strftime(timebuf, "%a, %d-%b-%Y %H:%M:%S GMT", tyme::gmtime(entry.m_dateTime) );
							printf("    %s\n", timebuf);
						}

						if (!entry.m_author.m_name.empty() || !entry.m_author.m_email.empty())
						{
							std::cout << "    by " << entry.m_author.m_name;
							if (!entry.m_author.m_email.empty())
								std::cout << " <" << entry.m_author.m_email << ">" << std::endl;
						}
						if (entry.m_categories.size() > 0)
						{
							std::cout << "    in:";
							bool first = true;
							for (auto& category: entry.m_categories)
							{
								if (first) first = false;
								else std::cout << ",";
								std::cout << " " << category;
							};
							std::cout << std::endl;
						}
						if (!entry.m_description.empty() || !entry.m_content.empty() || entry.m_enclosures.size() > 0)
							std::cout << std::endl;

						if (!entry.m_description.empty()) std::cout << "    " << entry.m_description << std::endl;
						if (!entry.m_content.empty()) std::cout << "    " << entry.m_content << std::endl;
						if (entry.m_enclosures.size() > 0)
						{
							std::cout << "    enclosures:" << std::endl;
							for (auto& enclosure : entry.m_enclosures)
							{
								std::cout << "        " << enclosure.m_type << ": " << enclosure.m_url << " ";

								size_t s = enclosure.m_size * 10;
								const char* post = "B";
								if (s > 10240)
								{
									s /= 1024;
									post = "KiB";
									if (s > 10240)
									{
										s /= 1024;
										post = "MiB";
										if (s > 10240)
										{
											s /= 1024;
											post = "GiB";
										}
									}
								}

								std::cout << (s / 10);
								if (s % 10)
									std::cout << "." << (s % 10);
								std::cout << post << " (" << enclosure.m_size << ")" << std::endl;
							};
						}
						std::cout << std::endl;
					}

				}
				else
					dom::Print(xml->documentElement(), true);
			}
			else
				fwrite(xhr->getResponseText(), xhr->getResponseTextLength(), 1, stdout);
			printf("\n");
#ifdef WIN32
			SetConsoleCP(win32_cp);
#endif
		}
	}
public:
	OnReadyStateChange(): m_read(0) {}
};

int fetch(int argc, char* argv[], const db::ConnectionPtr&)
{
	if (argc < 2)
	{
		fprintf(stderr, "fetch: not enough params\n");
		fprintf(stderr, "fetch <url>\n");
		return 1;
	}

	printf("URL: %s\n", argv[1]);

	auto xhr = http::XmlHttpRequest::Create();
	if (xhr.get())
	{
		OnReadyStateChange rsc;
		//xhr->setDebug();
		xhr->onreadystatechange(&rsc);
		xhr->open(http::HTTP_GET, argv[1], false);
		xhr->send();
	}

	return 0;
}

void printOutline(const opml::Outline& outline, size_t depth = 0)
{
	std::string pre;
	for (size_t i = 0; i < depth; ++i) pre += "    ";
	std::cout << pre << outline.m_text;
	if (!outline.m_url.empty())
	{
		std::cout << " [" << outline.m_url << "]";
	}
	std::cout << std::endl;

	++depth;
	for (auto&& child: outline.m_children)
		printOutline(child, depth);
}

int opml_cmd(int argc, char* argv[], const db::ConnectionPtr&)
{
	if (argc < 2)
	{
		fprintf(stderr, "opml: not enough params\n");
		fprintf(stderr, "opml <file>\n");
		return 1;
	}

	printf("PATH: %s\n", argv[1]);

	dom::XmlDocumentPtr doc = dom::XmlDocument::fromFile(argv[1]);

	if (doc.get())
	{
		opml::Outline outline;
		if (opml::parse(doc, outline))
		{
			printOutline(outline);
		};
	}

	return 0;
}

class Ticker : public http::XmlHttpRequest::OnReadyStateChange
{
	size_t m_read;
	void onReadyStateChange(http::XmlHttpRequest* xhr)
	{
		putc('.', stdout); fflush(stdout);
	}
public:
	Ticker() : m_read(0) {}
};

enum class FEED {
	SITE_ATOM,
	ATOM,
	CLASS,
	SITE_RSS = CLASS,
	RSS,
};

std::ostream& operator << (std::ostream& o, FEED type)
{
	switch (type)
	{
	case FEED::SITE_ATOM: return o << "Atom (site)";
	case FEED::ATOM: return o << "Atom";
	case FEED::SITE_RSS: return o << "RSS (site)";
	case FEED::RSS: return o << "RSS";
	}
	return o << (int)type;
}

struct Link
{
	std::string href;
	std::string title;
	std::string feed;
	FEED type;
};

std::string htmlTitle(const dom::XmlDocumentPtr& doc)
{
	putc('#', stdout); fflush(stdout);
	std::string title;
	auto titles = doc->getElementsByTagName("title");
	if (titles && titles->length())
	{
		auto e = titles->element(titles->length() - 1);
		title = std::trim(e->innerText());
	}

	return title;
}

std::string getFeedTitle(const http::XmlHttpRequestPtr& xhr, const std::string& url)
{
	putc('-', stdout); fflush(stdout);
	if (!xhr)
		return std::string();

	xhr->open(http::HTTP_GET, url, false);
	xhr->send();
	auto xml = xhr->getResponseXml();
	if (!xml)
		return std::string();

	feed::Feed feed;
	if (!feed::parse(xml, feed))
		return std::string();

	return std::move(feed.m_feed.m_title);
}

std::vector<Link> feedLinks(const http::XmlHttpRequestPtr& xhr, const dom::XmlDocumentPtr& doc, const Uri& base, const std::string& pageTitle)
{
	putc('#', stdout); fflush(stdout);
	std::vector<Link> out;

	auto links = doc->getElementsByTagName("link");
	if (!links)
		return out;

	size_t count = links->length();
	for (size_t i = 0; i < count; ++i)
	{
		auto e = links->element(i);
		if (!e) continue;
		auto rel = e->getAttributeNode("rel");
		if (!rel) continue;

		bool alternate = false;
		bool home = false;
		auto node = std::tolower(rel->nodeValue());
		if (node == "alternate")
			alternate = true;
		else if (node == "alternate home")
			home = alternate = true;

		if (!alternate) continue;

		std::string type = e->getAttribute("type");
		std::tolower(type);

		if (type != "text/xml" && type != "application/rss+xml" && type != "application/atom+xml")
			continue;

		Link link =
		{
			Uri::canonical(e->getAttribute("href"), base).string(),
			e->getAttribute("title"),
			std::string(),
			(type == "application/atom+xml") ? (home ? FEED::SITE_ATOM : FEED::ATOM) : (home ? FEED::SITE_RSS : FEED::RSS)
		};

		if (link.title.empty())
			link.title = pageTitle;

		link.feed = getFeedTitle(xhr, link.href);
		out.push_back(link);
	}

	std::stable_sort(out.begin(), out.end(), [](const Link& lhs, const Link& rhs){
		return lhs.type < rhs.type;
	});
	return out;
}

std::vector<Link> reduce(const std::vector<Link>& list)
{
	std::vector<Link> links;

	for (auto&& link : list)
	{
		// if there are two links with the same feed title
		// and the type is different, prefer SITE_ATOM to SITE_RSS
		// and ATOM to RSS
		if (link.feed.empty())
		{
			links.push_back(link);
			continue;
		}

		bool found = false;
		for (auto&& check : links)
		{
			// this will add an RSS link, even if there is already SITE_ATOM with the same title,
			// but will not add SITE_RSS in this situation, nor will it add the RSS link if there
			// already is an ATOM one.
			if (check.feed == link.feed && // same feed title
				check.type < link.type && // and the added link has higher type
				((int)link.type - (int)check.type) == (int)FEED::CLASS) // and the types are exactly class apart
			{
				found = true; // don't add this link
				break;
			}
		}

		if (found) continue;
		links.push_back(link);
	}

	return links;
}

void fixTitles(std::vector<Link>& links)
{
	size_t count = links.size();
	for (size_t i = 0; i < count; ++i)
	{
		Link& link = links[i];
		if (link.feed.empty()) continue;

		bool atLeastOneChanged = false;
		for (size_t j = i + 1; j < count; ++j)
		{
			Link& check = links[j];
			if (link.feed == check.feed)
			{
				atLeastOneChanged = true;
				if (!check.title.empty())
					check.feed += " (" + check.title + ")";
			}
		}

		if (atLeastOneChanged)
		{
			if (!link.title.empty())
				link.feed += " (" + link.title + ")";
		}
	}

	for (auto&& link : links)
	{
		if (link.feed.empty())
			link.feed = link.title;

		if (link.feed.empty())
			link.feed = "(no title)";
	}
}

int discover(int argc, char* argv[], const db::ConnectionPtr&)
{
	if (argc < 2)
	{
		fprintf(stderr, "discover: not enough params\n");
		fprintf(stderr, "discover <url>\n");
		return 1;
	}

	auto xhr = http::XmlHttpRequest::Create();
	if (!xhr)
		return 1;

	Ticker tick;
	xhr->onreadystatechange(&tick);
	xhr->open(http::HTTP_GET, argv[1], false);
	xhr->send();

	auto doc = xhr->getResponseXml();
	if (!doc)
		doc = xhr->getResponseHtml();
	if (doc)
	{
		std::string title = htmlTitle(doc);

		auto base = Uri::make_base(argv[1]);

		auto links = feedLinks(xhr, doc, base, title);
		putc('#', stdout); fflush(stdout);
		printf("\n");

		for (auto&& link : links)
			std::cout << '[' << link.type << "] \"" << link.feed << "\" (" << link.title << ") " << link.href << "\n";

		links = reduce(links);
		printf("# reduced...\n"); fflush(stdout);

		fixTitles(links);

		if (links.empty())
			std::cout << "Warning: no feeds found in the page\n";

		for (auto&& link : links)
			std::cout << '[' << link.type << "] \"" << link.feed << "\" " << link.href << "\n";
	}

	return 0;
}
