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
#include <sanitize.hpp>
#include <dom/dom.hpp>
#include <dom/parsers/html.hpp>
#include <sstream>
#include <utils.hpp>
#include <uri.hpp>
#include <vector>
#include <config.hpp>

namespace std
{
	vector<string> operator / (const string& list, const string& sep)
	{
		vector<string> out;
		std::string::size_type start = 0, pos = 0;
		do
		{
			pos = list.find(sep, start);
			if (pos == std::string::npos)
				out.push_back(list.substr(start));
			else
				out.push_back(list.substr(start, pos - start));

			start = pos + sep.length();
		} while (pos != std::string::npos);

		return out;
	}
	vector<string> operator / (const string& list, char sep)
	{
		vector<string> out;
		std::string::size_type start = 0, pos = 0;
		do
		{
			pos = list.find(sep, start);
			if (pos == std::string::npos)
				out.push_back(list.substr(start));
			else
				out.push_back(list.substr(start, pos - start));

			start = pos + 1;
		} while (pos != std::string::npos);

		return out;
	}
}

namespace sanitize
{
	struct StringBuilder : public dom::parsers::OutStream
	{
		std::ostringstream stream;
		void putc(char c) override { stream.write(&c, 1); }
		void puts(const char* s, size_t length) override { stream.write(s, length); }

		std::string str() { return std::move(stream.str()); }
	};

	bool sanitize(std::string& dst, const std::string& html)
	{
		auto doc = dom::parsers::html::parseDocument(std::string(), html.c_str(), html.length());
		if (!doc)
			return false;

		StringBuilder stream;
		if (!sanitize(stream, doc))
			return false;

		dst = std::move(stream.str());
		return true;
	}

	enum SANITIZE
	{
		SANITIZE_ERROR,
		SANITIZE_CONTINUE,
		SANITIZE_REMOVE,
		SANITIZE_REPLACE
	};

	struct item_info
	{
		std::string element;
		std::string attr;
		SANITIZE action;
		bool optional;

		bool matches(const dom::ElementPtr& e) const
		{
			if (!element.empty() && element != e->tagName())
				return false;

			if (!attr.empty())
				return e->hasAttribute(attr);

			return true;
		}

		bool matchForHref(const std::string& tagName) const
		{
			if (!element.empty() && element != tagName)
				return false;

			return !attr.empty();
		}

		bool findHref(const dom::ElementPtr& e, std::string& value) const
		{
			auto atts = e->getAttributes();

			for (auto att : dom::list_atts(atts))
			{
				if (att->name() != attr)
					continue;

				value = att->value();
				return true;
			}

			return false;
		}
	};

	using items = std::vector<item_info>;

	struct SanitizeDB
	{
		items blacklisted;
		items hrefs;
		items redundant;

		void read(const filesystem::path& ini)
		{
			auto cfg = config::base::file_config(ini, false);
			read(blacklisted, cfg->get_section("blacklisted"));
			read(hrefs, cfg->get_section("hrefs"));
			read(redundant, cfg->get_section("redundant"));
		}

		static void read(items& dst, const config::base::section_ptr& section)
		{
			dst.clear();
			auto list = section->get_keys();
			for (auto&& name : list)
				push_back(dst, name, section->get_string(name, std::string()));
		}

		static void push_back(items& dst, const std::string& name, const std::string& value)
		{
			SANITIZE action = SANITIZE_REMOVE;
			bool optional = false;
			std::string tag, attr;

			auto match = name / '@';
			tag = std::tolower(std::trim((const std::string&)match[0]));
			if (match.size() > 1)
				attr = std::tolower(std::trim((const std::string&)match[1]));

			auto list = value / ',';
			for (auto token : list)
			{
				token = std::tolower(std::trim((const std::string&)token));
				if (token == "optional") optional = true;
				else if (token == "required") optional = false;
				else if (token == "remove") action = SANITIZE_REMOVE;
				else if (token == "replace") action = SANITIZE_REPLACE;
			}

			dst.push_back({ tag, attr, action, optional });
		}

		SANITIZE cleanBlacklisted(const dom::ElementPtr& e)
		{
			for (auto&& info : blacklisted)
			{
				if (info.matches(e))
				{
					if (info.attr.empty())
						return info.action;

					e->removeAttribute(info.attr);
				}
			}

			return SANITIZE_CONTINUE;
		}

		SANITIZE cleanHrefs(const dom::ElementPtr& e)
		{
			auto tag = e->tagName();
			for (auto&& info : hrefs)
			{
				if (!info.matchForHref(tag))
					continue;

				std::string href;
				if (!info.findHref(e, href))
				{
					if (info.optional)
						continue;

					return info.action;
				}

				Uri uri{ href };
				if (uri.opaque() || uri.relative())
					return info.action; // TODO: make absolute hierarchical URIs if possible
			}

			return SANITIZE_CONTINUE;
		}

		SANITIZE cleanRedundant(const dom::ElementPtr& e)
		{
			for (auto&& info : redundant)
			{
				if (!info.matches(e))
					continue;

				auto atts = e->getAttributes();
				if (atts && atts->length() == 0)
					return info.action;
			}

			return SANITIZE_CONTINUE;
		}

		SANITIZE sanitize(const dom::ElementPtr& e)
		{
			auto ret = cleanBlacklisted(e);
			if (ret != SANITIZE_CONTINUE)
				return ret;

			ret = cleanHrefs(e);
			if (ret != SANITIZE_CONTINUE)
				return ret;

			return cleanRedundant(e);
		}

		static SanitizeDB& instance()
		{
			static SanitizeDB db;
			return db;
		}
	};

	void init(const filesystem::path& ini)
	{
		SanitizeDB::instance().read(ini);
	}

	SANITIZE sanitize(const dom::ElementPtr& e)
	{
		return SanitizeDB::instance().sanitize(e);
	}

	void startQueue(std::list<dom::ElementPtr>& queue, const dom::DocumentPtr& doc)
	{
		auto e = doc->documentElement();
		if (e)
			queue.push_back(e);
		auto f = doc->associatedFragment();
		if (f)
		{
			for (auto&& node : dom::list_elements(f->childNodes()))
			{
				if (!node) continue;
				queue.push_back(node);
			}
		}
	}

	void pushChildren(std::list<dom::ElementPtr>& queue, const dom::NodeListPtr& children)
	{
		for (auto&& e : dom::list_elements(children))
		{
			if (e)
				queue.push_back(e);
		}
	}

	enum CLEAN
	{
		CLEAN_ERROR,
		CLEAN_DEEPER,
		CLEAN_SKIP
	};

	CLEAN cleanElement(const dom::ElementPtr& e, const dom::NodeListPtr& replacement)
	{
		switch (SanitizeDB::instance().sanitize(e))
		{
		case SANITIZE_ERROR:   return CLEAN_ERROR;
		case SANITIZE_REMOVE:  return e->remove() ? CLEAN_SKIP : CLEAN_ERROR;
		case SANITIZE_REPLACE: return e->replace(replacement) ? CLEAN_DEEPER : CLEAN_ERROR;
		default:
			break;
		}

		return CLEAN_DEEPER;
	}

	bool sanitize(const dom::DocumentPtr& doc)
	{
		std::list<dom::ElementPtr> queue;
		startQueue(queue, doc);

		while (!queue.empty())
		{
			auto e = queue.front();
			queue.pop_front();

			auto children = e->childNodes();
			auto clean = cleanElement(e, children);
			if (clean == CLEAN_ERROR)
				return false;
			if (clean == CLEAN_SKIP)
				continue;

			pushChildren(queue, children);
		}

		return true;
	}

	bool sanitize(dom::parsers::OutStream& stream, const dom::DocumentPtr& document)
	{
		if (!sanitize(document))
			return false;

		dom::parsers::html::serialize(stream, document);
		return true;
	}

}
