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
#include "avatar.hpp"
#include <http.hpp>
#include <crypt.hpp>
#include <filesystem.hpp>

namespace gd
{
	template <typename T>
	struct unique_array
	{
		T* ptr = nullptr;
		size_t size = 0;
		bool resize(size_t size)
		{
			T* tmp = (T*)realloc(ptr, size);
			if (!tmp)
				return false;

			ptr = tmp;
			this->size = size;
			return true;
		}
		bool grow()
		{
			if (!size)
				return resize(10240);
			return resize(size << 1);
		}
		~unique_array() { free(ptr); }
	};

	class File
	{
		FILE* f;

	public:
		explicit File(FILE* f) : f(f) {}
		~File() { if (f) fclose(f); }
		explicit operator bool() const { return f != nullptr; }

		size_t read(char* buffer, size_t size)
		{
			return fread(buffer, 1, size, f);
		}
	};

	gdImagePtr loadImage(int size, void* data)
	{
#if 0
		Crypt::MD5Hash::digest_t hash;
		char salt[20];
		Crypt::newSalt(salt);
		if (Crypt::MD5Hash::simple(salt, sizeof(salt), hash))
		{
			std::string path;
			static const char alphabet[] = "0123456789abcdef";
			for (auto d : hash)
			{
				path.push_back(alphabet[(d >> 4) & 0xF]);
				path.push_back(alphabet[d & 0xF]);
			}
			path += ".png";
			auto fname = filesystem::canonical(path);
			FLOG << "Icon saved as: " << fname;
			auto f = fopen(fname.native().c_str(), "wb");
			if (f)
			{
				fwrite(ptr->getResponseText(), 1, ptr->getResponseTextLength(), f);
				fclose(f);
			}
			else
			{
				FLOG << "   Could not open.";
			}
		}
#endif

#ifdef WIN32
#define CALLTYPE _stdcall
#else
#define CALLTYPE
#endif
		using creator_t = gdImagePtr(CALLTYPE*)(int size, void *data);
		creator_t creators[] = {
			gdImageCreateFromPngPtr,
			gdImageCreateFromJpegPtr,
			gdImageCreateFromGifPtr
		};

		for (auto&& creator : creators)
		{
			auto ret = creator(size, data);
			if (ret)
				return ret;
		}
		return nullptr;
	}

	gdImagePtr loadImage(const filesystem::path& path)
	{

		File f{ fopen(path.native().c_str(), "rb") };
		if (!f)
			return nullptr;

		filesystem::status st{ path };
		if (!st.exists())
			return nullptr;

		unique_array<char> buffer;
		if (!buffer.resize((size_t)st.file_size()))
			return nullptr;

		if (f.read(buffer.ptr, buffer.size) != buffer.size)
			return nullptr;

		return loadImage(buffer.size, buffer.ptr);
	}

	gdImagePtr loadImage(const http::XmlHttpRequestPtr& ptr)
	{
		return loadImage(ptr->getResponseTextLength(), (char*)ptr->getResponseText());
	}
}

namespace FastCGI { namespace avatar {

	IconEnginePtr Engines::_handler(Request& request)
	{
		auto session = request.getSession(false);
		std::string id;
		if (session)
			id = session->profile()->avatarEngine();
		if (request.getVariable("force_avatar"))
			id = request.getVariable("force_avatar");

		auto _it = m_handlers.find(id);
		if (_it == m_handlers.end()) _it = m_handlers.find("default");
		if (_it == m_handlers.end()) return nullptr;
		return engine(*_it);
	}

	class DefaultEngine : public IconEngine
	{
	public:
		const char* name() const override { return nullptr; }
		const char* homePage() const override { return nullptr; }
		GdImage loadIcon(const SessionPtr& session, Request& request, int max_size) override;
	};

	class GravatarEngine : public IconEngine
	{
	public:
		const char* name() const override { return "Gravatar"; }
		const char* homePage() const override { return "http://www.gravatar.com/"; }
		GdImage loadIcon(const SessionPtr& session, Request& request, int max_size) override;
	};

	GdImage DefaultEngine::loadIcon(const SessionPtr& session, Request& request, int max_size)
	{
		GdImage img{ gd::loadImage(request.app().getDataDir() / "images/public.png") };
		if (max_size > 0)
			img.resample(max_size, max_size);

		return img;
	}

	GdImage GravatarEngine::loadIcon(const SessionPtr& session, Request& request, int max_size)
	{
		if (session)
		{
			auto email = std::tolower(session->profile()->email());
			Crypt::MD5Hash::digest_t hash;
			if (Crypt::MD5Hash::simple(email.c_str(), email.length(), hash))
			{
				std::string url = "http://www.gravatar.com/avatar/";
				static const char alphabet[] = "0123456789abcdef";
				for (auto d : hash)
				{
					url.push_back(alphabet[(d >> 4) & 0xF]);
					url.push_back(alphabet[d & 0xF]);
				}

				if (max_size > 0)
				{
					url.append("?s=");
					url.append(std::to_string(max_size));
				}

#if 0
				url.push_back(max_size > 0 ? '&' : '?');
				url.append("d=404");
#endif

				auto xhr = http::XmlHttpRequest::Create();
				xhr->open(http::HTTP_GET, url, false);
				xhr->send();
				int statusClass = xhr->getStatus() / 100;
				if (statusClass == 2)
				{
					GdImage img{ gd::loadImage(xhr) };
					if (img)
					{
						if (max_size > 0) // in case the service didn't do it..
							img.resample(max_size, max_size);

						return img;
					}
				}
			}
		}

		DefaultEngine fallback;
		return fallback.loadIcon(session, request, max_size);
	}

}}

REGISTER_ICON_ENGINE("default", FastCGI::avatar::DefaultEngine);
REGISTER_ICON_ENGINE("gravatar", FastCGI::avatar::GravatarEngine);
