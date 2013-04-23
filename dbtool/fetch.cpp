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

#include <dbconn.hpp>
#include <utils.hpp>

#include <http.hpp>
#include <feed_parser.hpp>
#include <opml.hpp>

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
			std::for_each(headers.begin(), headers.end(), [](const std::pair<std::string, std::string>& value)
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
			});
			printf("\n");
		}

		if (state == http::XmlHttpRequest::DONE && (xhr->getStatus() / 100 == 2))
		{
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
					if (!feed.m_description.empty()) std::cout << feed.m_description << std::endl;
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

	auto cur = outline.m_children.begin(), end = outline.m_children.end();
	++depth;
	for (; cur != end; ++cur)
		printOutline(*cur, depth);
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
