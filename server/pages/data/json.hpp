/*
 * Copyright (C) 2013 Aggregate
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

#ifndef __JSON_HPP__
#define __JSON_HPP__

#include <fast_cgi/request.hpp>

namespace json
{
	std::string escape(const char* in);
	std::string escape(const std::string& in);

	template <typename Type> struct Json;

	template <typename T>
	static inline bool render(FastCGI::Request& request, const T& t)
	{
		return Json<T>(t).render(request);
	}

	struct JsonBase
	{
		JsonBase(const void* ctx): m_ctx(ctx) {}
		virtual bool render(FastCGI::Request& request) = 0;
	protected:
		const void* m_ctx;
	};

	template <>
	struct Json<std::string>: JsonBase
	{
		Json(const std::string& ctx): JsonBase(&ctx) {}
		bool render(FastCGI::Request& request)
		{
			const std::string* ctx = (const std::string*)m_ctx;
			if (!ctx)
				return false;

			request << escape(*ctx);
			return true;
		}
	};

	template <>
	struct Json<const char*>: JsonBase
	{
		Json(const char* const& ctx): JsonBase(ctx) {}
		bool render(FastCGI::Request& request)
		{
			const char* ctx = (const char*)m_ctx;

			request << escape(ctx);
			return true;
		}
	};

	template <>
	struct Json<long long>: JsonBase
	{
		Json(const long long& ctx): JsonBase(&ctx) {}
		bool render(FastCGI::Request& request)
		{
			const long long* ctx = (const long long*)m_ctx;
			if (!ctx)
				return false;

			request << *ctx;
			return true;
		}
	};

	template <>
	struct Json<long>: JsonBase
	{
		Json(const long& ctx): JsonBase(&ctx) {}
		bool render(FastCGI::Request& request)
		{
			const long* ctx = (const long*)m_ctx;
			if (!ctx)
				return false;

			request << *ctx;
			return true;
		}
	};

	template <>
	struct Json<int>: JsonBase
	{
		Json(const int& ctx): JsonBase(&ctx) {}
		bool render(FastCGI::Request& request)
		{
			const int* ctx = (const int*)m_ctx;
			if (!ctx)
				return false;

			request << *ctx;
			return true;
		}
	};

	template <>
	struct Json<short>: JsonBase
	{
		Json(const short& ctx): JsonBase(&ctx) {}
		bool render(FastCGI::Request& request)
		{
			const short* ctx = (const short*)m_ctx;
			if (!ctx)
				return false;

			request << *ctx;
			return true;
		}
	};

	template <>
	struct Json<char>: JsonBase
	{
		Json(const char& ctx): JsonBase(&ctx) {}
		bool render(FastCGI::Request& request)
		{
			const char* ctx = (const char*)m_ctx;
			if (!ctx)
				return false;

			request << (int)*ctx;
			return true;
		}
	};

	template <typename T>
	struct Json< std::list<T> >: JsonBase
	{
		Json(const std::list<T>& ctx): JsonBase(&ctx) {}
		bool render(FastCGI::Request& request)
		{
			std::list<T>* ctx = (std::list<T>*)m_ctx;
			if (!ctx)
				return false;
			request << "[";
			bool first = true;
			auto it = std::find_if(ctx->begin(), ctx->end(), [&first, &request](const T& t) -> bool
			{
				if (first) first = false;
				else request << ",";

				return !json::render(request, t);
			});
			request << "]";
			return it == ctx->end();
		}
	};

	struct ColumnSelectorBase
	{
		virtual ~ColumnSelectorBase() {}
		virtual bool render(FastCGI::Request& request, const db::CursorPtr& c) = 0;
		virtual std::string name() const = 0;
	};
	typedef std::shared_ptr<ColumnSelectorBase> ColumnSelectorBasePtr;

	template <typename Member>
	struct ColumnSelector: ColumnSelectorBase
	{
		std::string m_name;
		int m_column;

		ColumnSelector(const std::string& name, int column)
			: m_name(name)
			, m_column(column)
		{
		}

		bool render(FastCGI::Request& request, const db::CursorPtr& c)
		{
			const Member& new_ctx = db::Selector<Member>::get(c, m_column);
			return json::render(request, new_ctx);
		}
		std::string name() const { return m_name; }
	};

	template <typename Type>
	struct CursorJson: JsonBase
	{
		CursorJson(const db::CursorPtr& c): JsonBase(&c) {}
		std::list<ColumnSelectorBasePtr> m_selectors;
		template <typename Member>
		void add(const std::string& name, int column)
		{
			m_selectors.push_back(std::make_shared< ColumnSelector<Member> >(name, column));
		}

		bool entireCursor() const { return true; }

		bool renderCurrent(FastCGI::Request& request, const db::CursorPtr& c)
		{
			request << "{";
			bool first = true;
			auto cur = m_selectors.begin(), end = m_selectors.end();
			for (; cur != end; ++cur)
			{
				if (first) first = false;
				else request << ",";

				ColumnSelectorBasePtr& selector = *cur;
				request << escape(selector->name()) << ":";
				if (!selector->render(request, c))
					return false;
			}
			request << "}";
			return true;
		}

		bool render(FastCGI::Request& request)
		{
			const db::CursorPtr& c = *(db::CursorPtr*)m_ctx;

			if (!static_cast<Type*>(this)->entireCursor())
				return renderCurrent(request, c);

			request << "[";
			bool first = true;
			while (c->next())
			{
				if (first) first = false;
				else request << ",";

				if (!renderCurrent(request, c))
					return false;
			}
			request << "]";
			return true;
		};
	};

	struct MemberSelectorBase
	{
		virtual ~MemberSelectorBase() {}
		virtual bool render(FastCGI::Request& request, const void* context) = 0;
		virtual std::string name() const = 0;
	};
	typedef std::shared_ptr<MemberSelectorBase> MemberSelectorBasePtr;

	template <typename Type, typename Member>
	struct MemberSelector: MemberSelectorBase
	{
		std::string m_name;
		Member Type::* m_member;

		MemberSelector(const std::string& name, Member Type::* member)
			: m_name(name)
			, m_member(member)
		{
		}

		bool render(FastCGI::Request& request, const void* context)
		{
			const Type* ctx = (const Type*)context;
			if (!ctx)
				return false;

			const Member& new_ctx = ctx->*m_member;

			return json::render(request, new_ctx);
		}
		std::string name() const { return m_name; }
	};

	template <typename Type, typename CursorRenderer>
	struct MemberCursorSelector: MemberSelectorBase
	{
		std::string m_name;
		db::CursorPtr Type::* m_member;

		MemberCursorSelector(const std::string& name, db::CursorPtr Type::* member)
			: m_name(name)
			, m_member(member)
		{
		}

		bool render(FastCGI::Request& request, const void* context)
		{
			const Type* ctx = (const Type*)context;
			if (!ctx)
				return false;

			const db::CursorPtr& c = ctx->*m_member;
			return CursorRenderer(c).render(request);
		}
		std::string name() const { return m_name; }
	};

	template <typename Type>
	struct StructJson: JsonBase
	{
		StructJson(const Type& ctx): JsonBase(&ctx) {}
		std::list<MemberSelectorBasePtr> m_selectors;
		template <typename Member>
		void add(const std::string& name, Member Type::* dest)
		{
			m_selectors.push_back(std::make_shared< MemberSelector<Type, Member> >(name, dest));
		}

		template <typename CursorRenderer>
		void addCursor(const std::string& name, db::CursorPtr Type::* dest)
		{
			m_selectors.push_back(std::make_shared< MemberCursorSelector<Type, CursorRenderer> >(name, dest));
		}

		bool render(FastCGI::Request& request)
		{
			request << "{";
			bool first = true;
			auto cur = m_selectors.begin(), end = m_selectors.end();
			for (; cur != end; ++cur)
			{
				if (first) first = false;
				else request << ",";

				MemberSelectorBasePtr& selector = *cur;
				request << escape(selector->name()) << ":";
				if (!selector->render(request, m_ctx))
					return false;
			}
			request << "}";
			return true;
		};
	};
}

#define JSON_RULE(type) \
	template <> \
	struct Json<type>: StructJson<type> \
	{ \
		typedef type Type; \
		Json(const Type& ctx); \
	}; \
	Json<type>::Json(const Type& ctx): StructJson<type>(ctx)
#define JSON_ADD2(name, dest) add(#name, &Type::dest)
#define JSON_ADD(name) add(#name, &Type::name)
#define JSON_ADD_CURSOR(name, type) addCursor<type>(#name, &Type::name)

#define JSON_CURSOR_RULE(type) \
	struct type: CursorJson<type> \
	{ \
		type(const db::CursorPtr& c); \
	}; \
	type::type(const db::CursorPtr& c): CursorJson<type>(c)
#define JSON_CURSOR_ADD(name, column, type) add<type>(#name, (column))
#define JSON_CURSOR_TEXT(name, column) add<const char*>(#name, (column))
#define JSON_CURSOR_L(name, column) add<long>(#name, (column))
#define JSON_CURSOR_LL(name, column) add<long long>(#name, (column))

#endif //__JSON_HPP__