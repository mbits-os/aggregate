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

#ifndef __DBCONN_SCHEMA_H__
#define __DBCONN_SCHEMA_H__

#include <dbconn.hpp>

#include <string>
#include <list>

namespace db
{
	namespace model
	{
		enum class FIELD_TYPE
		{
			KEY,
			TEXT_KEY,
			INTEGER,
			TEXT,
			BOOLEAN,
			TIME,
			BLOB
		};

		enum class att
		{
			NONE            = 0x0000,
			NOTNULL         = 0x0001,
			AUTOINCREMENT   = 0x0002,
			UNIQUE		    = 0x0004,
			KEY             = 0x0008,
			REFERENCES      = 0x0010,
			DELETE_RESTRICT = 0x0020,
			UPDATE_RESTRICT = 0x0040,
			DELETE_CASCADE  = 0x0080,
			UPDATE_CASCADE  = 0x0100,
			DELETE_NULLIFY  = 0x0200,
			UPDATE_NULLIFY  = 0x0400,
			DEFAULT         = 0x0800
		};

		struct Attributes
		{
			unsigned int value = 0;
			Attributes() {}
			Attributes(att att): value((unsigned int)att) {}
			Attributes& operator |= (att att)
			{
				value |= (unsigned int)att;
				return *this;
			}

			bool operator & (att att) const
			{
				return !!(value & ((unsigned int)att));
			}
		};

		inline Attributes operator|(const Attributes& attributes, att att)
		{
			auto tmp = attributes;
			return tmp |= att;
		}

		inline Attributes operator|(att left, att right)
		{
			return Attributes(left) | right;
		}

		class Field
		{
			std::string m_name;
			FIELD_TYPE m_fld_type;
			Attributes m_attributes;
			std::string m_ref;
		public:
			Field(const std::string& name, FIELD_TYPE fld_type, Attributes attributes, const std::string& ref = std::string())
				: m_name(name)
				, m_fld_type(fld_type)
				, m_attributes(attributes)
				, m_ref(ref)
			{
			}
			std::string repr() const;
			void constraints(std::list<std::string>& cos) const;
		};

		class Table
		{
			std::string m_name;
			std::list<Field> m_fields;
		public:
			Table(const std::string& name): m_name(name) {}
			Table& field(const std::string& name, const std::string& defValue = std::string(), FIELD_TYPE type = FIELD_TYPE::TEXT,
				Attributes attributes = att::NOTNULL)
			{
				if (!defValue.empty())
					attributes |= att::DEFAULT;

				m_fields.push_back(Field(name, type, attributes, defValue));
				return *this;
			}
			Table& nullable(const std::string& name, const std::string& defValue = std::string(), FIELD_TYPE type = FIELD_TYPE::TEXT,
				Attributes attributes = Attributes())
			{
				if (!defValue.empty())
					attributes |= att::DEFAULT;

				m_fields.push_back(Field(name, type, attributes, defValue));
				return *this;
			}
			Table& _id()
			{
				m_fields.push_back(Field("_id", FIELD_TYPE::KEY, att::NOTNULL | att::AUTOINCREMENT | att::KEY));
				return *this;
			}
			Table& text_id(const std::string& name)
			{
				m_fields.push_back(Field(name, FIELD_TYPE::TEXT_KEY, att::NOTNULL | att::KEY));
				return *this;
			}
			Table& blob(const std::string& name)
			{
				m_fields.push_back(Field(name, FIELD_TYPE::BLOB, Attributes()));
				return *this;
			}
			Table& refer(const std::string& remote)
			{
				m_fields.push_back(Field(remote + "_id", FIELD_TYPE::KEY, att::NOTNULL | att::REFERENCES | att::DELETE_CASCADE, remote));
				return *this;
			}
			std::string drop() const;
			std::string create() const;
		};

		class View
		{
			std::string m_name;
			std::string m_select;
		public:
			View(const std::string& name, const std::string& select): m_name(name), m_select(select) {}
			std::string drop() const;
			std::string create() const;
		};

		class SchemaDefinition
		{
			std::list<Table> m_tables;
			std::list<View> m_views;
		public:
			Table& table(const std::string& name)
			{
				m_tables.push_back(name);
				return m_tables.back();
			}
			View& view(const std::string& name, const std::string& select)
			{
				m_views.push_back(View(name, select));
				return m_views.back();
			}

			template <typename Cont, typename T>
			static void each(std::list<std::string>& out, const Cont& cont, std::string (T::*op)() const)
			{
				for (const T& item : cont)
				{
					out.push_back((item.*op)());
				}
			}

			template <typename Cont>
			struct reverse_t
			{
				const Cont& ref;
			public:
				reverse_t(const Cont& ref) : ref(ref) {}
				typename Cont::const_reverse_iterator begin() const { return ref.rbegin(); }
				typename Cont::const_reverse_iterator end() const { return ref.rend(); }
			};

			template <typename Cont>
			static reverse_t<Cont> reverse(const Cont& cont) { return reverse_t<Cont>(cont); }

			std::list<std::string> drop() const
			{
				std::list<std::string> out;
				each(out, reverse(m_views), &View::drop);
				each(out, reverse(m_tables), &Table::drop);
				return out;
			}
			std::list<std::string> create() const
			{
				std::list<std::string> out;
				each(out, m_tables, &Table::create);
				each(out, m_views, &View::create);
				return out;
			}

			static SchemaDefinition& schema();
		};

		struct User
		{
			std::string login;
			std::string name;
			std::string email;
		};

		using Users = std::vector<User>;

		class Schema
		{
			ConnectionPtr m_conn;
		public:
			Schema(const ConnectionPtr& conn) : m_conn(conn) {}
			bool install();
			bool addUser(const char* login, const char* mail, const char* name);
			bool removeUser(const char* mail);
			bool changePasswd(const char* mail, const char* passwd);
			bool getUsers(Users& users);
			long version();
			bool version(long);
		};
	}
};

#endif //__DBCONN_SCHEMA_H__
