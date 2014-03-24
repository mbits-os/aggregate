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

	const char* blacklisted_elements[] = {
		"title",
		"meta",
		"link",
		"script",
		"style",
		"embed",
		"object",
		"iframe"
	};

	const char* blacklisted_attrs[] = {
		"class",
	};

	struct
	{
		const char* tag;
		const char* attr;
		bool optional;
		SANITIZE action;
	} delicate[] = {
		{ "a", "href", false, SANITIZE_REPLACE },
		{ "img", "src", false, SANITIZE_REMOVE },
		{ "audio", "src", true, SANITIZE_REMOVE },
		{ "video", "src", true, SANITIZE_REMOVE },
		{ "source", "src", false, SANITIZE_REMOVE },
	};

	class sanitizer
	{
		std::ostringstream o;
	};

	SANITIZE sanitize(const dom::ElementPtr& e)
	{
		auto name = std::tolower(e->tagName());
		for (auto&& blacklisted : blacklisted_elements)
		{
			if (name == blacklisted)
				return SANITIZE_REMOVE;
		}

		auto atts = e->getAttributes();
		for (auto att : dom::list_atts(atts))
		{
			auto name = std::tolower(att->name());
			if (name.substr(0, 2) == "on")
			{
				e->removeAttribute(att);
				continue;
			}

			for (auto&& blacklisted : blacklisted_attrs)
			{
				if (name == blacklisted)
				{
					e->removeAttribute(att);
					break;
				}
			}
		}

		for (auto&& link : delicate)
		{
			if (name != link.tag)
				continue;

			bool found = false;

			std::string attrName = link.attr, attr;
			auto atts = e->getAttributes();

			for (auto att : dom::list_atts(atts))
			{
				if (std::tolower(att->name()) != attrName)
					continue;

				found = true;
				attrName = att->name();
				attr = att->value();
				break;
			}

			if (!found)
			{
				if (link.optional)
					continue;

				return link.action;
			}

			Uri uri{ attr };
			if (uri.opaque() || uri.relative())
				return link.action; // TODO: make absolute hierarchical URIs if possible
		}

		if (name == "span")
		{
			atts = e->getAttributes();
			if (atts && atts->length() == 0)
				return SANITIZE_REPLACE;
		}

		return SANITIZE_CONTINUE;
	}

	bool sanitize(const dom::DocumentPtr& doc)
	{
		std::list<dom::ElementPtr> queue;
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

		while (!queue.empty())
		{
			auto e = queue.front();
			queue.pop_front();

			auto result = sanitize(e);

			auto children = e->childNodes();

			switch (result)
			{
			case SANITIZE_ERROR:
				return false;
			case SANITIZE_REMOVE:
				if (!e->remove())
					return false;
				continue;
			case SANITIZE_REPLACE:
				if (!e->replace(children))
					return false;
				break;
			}

			for (auto&& e : dom::list_elements(children))
			if (e) queue.push_back(e);
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
