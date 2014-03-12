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
#include "auth/auth.hpp"
#include "avatar.hpp"

namespace FastCGI { namespace app { namespace reader {

	static inline int GET_ALPHA(int C)
	{
		return C >> 24;
	}

	static inline int GET_RED(int C)
	{
		return (C >> 16) & 0xFF;
	}

	static inline int GET_GREEN(int C)
	{
		return (C >> 8) & 0xFF;
	}

	static inline int GET_BLUE(int C)
	{
		return C & 0xFF;
	}

	int SET_COLOR(int alpha, int R, int G, int B)
	{
		return (alpha << 24) | ((R & 0xFF) << 16) | ((G & 0xFF) << 8) | (B & 0xFF);
	}

	class IconHandler : public Handler
	{
	public:
		DEBUG_NAME("User Icon");

		void visit(Request& request) override
		{
			SessionPtr session = request.getSession(false);

			auto engine = avatar::Engines::handler(request);
			if (!engine)
				request.on500("No avatar engines, not even the default one");

			int size = -1;
			auto arg = request.getVariable("s");
			if (arg)
			{
				size = atoi(arg);
				if (size < 16)
					size = 16;
			}

			gd::GdImage main{ engine->loadIcon(session, request, size) };

			if (!main)
				request.on404();

			main.alphaBlending(true);

			gd::GdImage mask{ gd::loadPng(request.app().getDataDir() / "images/mask.png") };

			if (mask)
			{
				size_t w = main.width();
				size_t h = main.height();
				mask.alphaBlending(true);
				mask.saveAlpha(true);
				mask.resample(w, h);
				for (size_t y = 0; y < h; ++y)
				{
					for (size_t x = 0; x < w; ++x)
					{
						auto main_px = gdImageTrueColorPixel(main.get(), x, y);
						auto& mask_px = gdImageTrueColorPixel(mask.get(), x, y);
						mask_px = SET_COLOR(GET_ALPHA(mask_px), GET_RED(main_px), GET_GREEN(main_px), GET_BLUE(main_px));
					}
				}
				main.swap(mask);
			}

			size = 0;
			void* data = gdImagePngPtr(main.get(), &size);
			if (!data)
				request.on500("OOM while creating user icon");

			request.setHeader("Content-Type", "image/png");
			request.setHeader("Content-Length", std::to_string(size));

			request.cout().write((char*)data, size);

			gdFree(data);
		}
	};

}}} // FastCGI::app::reader

REGISTER_HANDLER("/icon.png", FastCGI::app::reader::IconHandler);
