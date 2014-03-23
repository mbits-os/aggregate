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

#ifndef __AVATAR_HPP__
#define __AVATAR_HPP__

#include <fast_cgi.hpp>
#include <gd.h>
#include <string>
#include <map>
#include <handlers.hpp>
#include <http/http.hpp>

namespace gd
{
	class GdImage
	{
		gdImagePtr img;

	public:
		explicit GdImage(gdImagePtr img) : img(img) {}
		GdImage() = delete;
		GdImage(const GdImage&) = delete;
		GdImage& operator=(const GdImage&) = delete;
		GdImage(GdImage&& oth)
			: img(oth.release())
		{
		}
		GdImage& operator=(GdImage&& oth)
		{
			std::swap(img, oth.img);
			return *this;
		}

		~GdImage() { if (img) gdImageDestroy(img); }
		explicit operator bool() const { return img != nullptr; }
		gdImagePtr get() { return img; }

		void reset(gdImagePtr newImg)
		{
			if (img)
				gdImageDestroy(img);
			img = newImg;
		}

		gdImagePtr release()
		{
			auto tmp = img;
			img = nullptr;
			return tmp;
		}

		void swap(GdImage& oth)
		{
			std::swap(img, oth.img);
		}

		// GD2
		size_t width() const { return img->sx; }
		size_t height() const { return img->sy; }

		void alphaBlending(bool alpha) { gdImageAlphaBlending(img, alpha ? 1 : 0); }
		void saveAlpha(bool alpha) { gdImageSaveAlpha(img, alpha ? 1 : 0); }

		int colorAllocate(int r, int g, int b, int a)
		{
			return gdImageColorAllocateAlpha(img, r, g, b, a);
		}

		int colorAllocate(int r, int g, int b)
		{
			return gdImageColorAllocate(img, r, g, b);
		}

		void copy(const GdImage& src, int dstX = 0, int dstY = 0, int srcX = 0, int srcY = 0, int w = -1, int h = -1)
		{
			if (w < 0) w = src.width();
			if (h < 0) h = src.width();
			gdImageCopy(img, src.img, dstX, dstY, srcX, srcY, w, h);
		}

		void rectangle(int color, int x = 0, int y = 0, int w = -1, int h = -1)
		{
			if (w < 0) w = width();
			if (h < 0) h = width();
			gdImageRectangle(img, x, y, x + w, y + h, color);
		}

		void fillRectangle(int color, int x = 0, int y = 0, int w = -1, int h = -1)
		{
			if (w < 0) w = width();
			if (h < 0) h = width();
			gdImageFilledRectangle(img, x, y, x + w, x + h, color);
		}

		void fill(int color, int x, int y)
		{
			gdImageFill(img, x, y, color);
		}

		void resample(int w, int h)
		{
			if (width() == w && height() == h)
				return;

			GdImage tmp{ gdImageCreateTrueColor(w, h) };
			int bck = tmp.colorAllocate(255, 255, 255, gdAlphaTransparent);
			tmp.alphaBlending(false);
			tmp.fillRectangle(bck);
			tmp.alphaBlending(true);
			tmp.saveAlpha(true);
			gdImageCopyResampled(tmp.img, img, 0, 0, 0, 0, w, h, width(), height());
			swap(tmp);
		}
	};

	gdImagePtr loadImage(int size, void* data);
	gdImagePtr loadImage(const filesystem::path& path);
	gdImagePtr loadImage(const http::XmlHttpRequestPtr& ptr);
};

namespace FastCGI { namespace avatar {

	using gd::GdImage;

	struct IconEngine
	{
		virtual ~IconEngine() {}
		virtual const char* name() const = 0;
		virtual const char* homePage() const = 0;
		virtual GdImage loadIcon(const SessionPtr& session, Request& request, int max_size) = 0;
	};

	using IconEnginePtr = std::shared_ptr<IconEngine>;

#if DEBUG_CGI
	struct IconEngineDbgInfo
	{
		IconEnginePtr ptr;
		const char* file;
		size_t line;
	};

	using EngineMap = std::map<std::string, IconEngineDbgInfo>;
#else
	using EngineMap = std::map<std::string, IconEnginePtr>;
#endif


	class Engines
	{
		static Engines& get()
		{
			static Engines instance;
			return instance;
		}

		EngineMap m_handlers;
		IconEnginePtr _handler(Request& request);
		void _register(const std::string& command, const IconEnginePtr& ptr
#if DEBUG_CGI
			, const char* file, size_t line
#endif
			)
		{
#if DEBUG_CGI
			IconEngineDbgInfo info;
			info.ptr = ptr;
#if defined(POSIX) || defined(NDEBUG)
			info.file = file + 6; // skip "../../" (or "..\..\")
#else
			info.file = file + BUILD_DIR_LEN;
#endif
			info.line = line;
			m_handlers[command] = info;
#else
			m_handlers[command] = ptr;
#endif
		}

	public:
		static IconEnginePtr handler(Request& request)
		{
			return get()._handler(request);
		}

		template <typename EngineImpl>
		static void registerEngine(const std::string& resource
#if DEBUG_CGI
			, const char* file, size_t line
#endif
			)
		{
			auto ptr = std::make_shared<EngineImpl>();
			get()._register(resource, ptr
#if DEBUG_CGI
				, file, line
#endif
				);
		}

		static const EngineMap& engines() { return get().m_handlers; }
		static const IconEnginePtr& engine(const EngineMap::value_type& pair)
		{
#if DEBUG_CGI
			return pair.second.ptr;
#else
			return pair.second;
#endif
		}

	};

	template <typename EngineImpl>
	struct EngineRegistrar
	{
		EngineRegistrar(const std::string& id
#if DEBUG_CGI
			, const char* file, size_t line
#endif
			)
		{
			Engines::registerEngine<EngineImpl>(id
#if DEBUG_CGI
				, file, line
#endif
				);
		}
	};
}} //FastCGI::avatar

#define REGISTER_ICON_ENGINE(id, type) static FastCGI::avatar::EngineRegistrar<type> REGISTRAR_NAME(icon_engine) (id REGISTRAR_ARGS)

#endif // __AVATAR_HPP__
