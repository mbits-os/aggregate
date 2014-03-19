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
#include <functional>

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

		namespace VERSION { enum { SAME_AS_PARENT = -1, CURRENT = 2 }; }

		static inline bool version_valid(long currVersion, long newVersion, long version)
		{
			return currVersion < version && version <= newVersion;
		}

		static inline bool should_drop(long currVersion, long newVersion, long max_version)
		{
			return currVersion <= max_version && max_version < newVersion;
		}

		class Field
		{
			std::string m_name;
			FIELD_TYPE m_fld_type;
			Attributes m_attributes;
			std::string m_ref;
			int m_length;
			long m_version;
			long m_max_version;
		public:
			Field(const std::string& name, FIELD_TYPE fld_type, Attributes attributes, const std::string& ref = std::string(), long version = VERSION::SAME_AS_PARENT)
				: m_name(name)
				, m_fld_type(fld_type)
				, m_attributes(attributes)
				, m_ref(ref)
				, m_length(-1)
				, m_version(version)
				, m_max_version(-1)
			{
			}
			bool altered(long currVersion, long newVersion) const { return version_valid(currVersion, newVersion, m_version) || should_drop(currVersion, newVersion, m_max_version); }
			long version() const { return m_version; }
			long max_version() const { return m_max_version; }
			void max_version(long value) { m_max_version = value; }
			const std::string& name() const { return m_name; }
			std::string repr(bool alter = false) const;
			void constraints(std::list<std::string>& cos) const;
			int length() const { return m_length; }
			void length(int len) { m_length = len; }
		};

		class Table
		{
			std::string m_name;
			std::list<Field> m_fields;
			long m_version;
		public:
			Table(const std::string& name, long version = 1) : m_name(name), m_version(version) {}
			Table& field(const std::string& name, const std::string& defValue = std::string(), FIELD_TYPE type = FIELD_TYPE::TEXT,
				Attributes attributes = att::NOTNULL, long version = VERSION::SAME_AS_PARENT)
			{
				if (!defValue.empty())
					attributes |= att::DEFAULT;

				m_fields.push_back(Field(name, type, attributes, defValue, version));
				return *this;
			}
			Table& nullable(const std::string& name, const std::string& defValue = std::string(), FIELD_TYPE type = FIELD_TYPE::TEXT,
				Attributes attributes = Attributes(), long version = VERSION::SAME_AS_PARENT)
			{
				if (!defValue.empty())
					attributes |= att::DEFAULT;

				m_fields.push_back(Field(name, type, attributes, defValue, version));
				return *this;
			}
			Table& _id()
			{
				m_fields.push_back(Field("_id", FIELD_TYPE::KEY, att::NOTNULL | att::AUTOINCREMENT | att::KEY));
				return *this;
			}
			Table& text_id(const std::string& name, long version = VERSION::SAME_AS_PARENT)
			{
				m_fields.push_back(Field(name, FIELD_TYPE::TEXT_KEY, att::NOTNULL | att::KEY, std::string(), version));
				return *this;
			}
			Table& blob(const std::string& name, long version = VERSION::SAME_AS_PARENT)
			{
				m_fields.push_back(Field(name, FIELD_TYPE::BLOB, Attributes(), std::string(), version));
				return *this;
			}
			Table& refer(const std::string& remote, long version = VERSION::SAME_AS_PARENT)
			{
				m_fields.push_back(Field(remote + "_id", FIELD_TYPE::KEY, att::NOTNULL | att::REFERENCES | att::DELETE_CASCADE, remote, version));
				return *this;
			}

			Table& add(const Field& field)
			{
				m_fields.push_back(field);
				return *this;
			}

			Table& max_version(const std::string& name, long value)
			{
				for (auto&& fld : m_fields)
				{
					if (fld.name() == name)
					{
						fld.max_version(value);
						break;
					}
				}
				return *this;
			}

			long version() const { return m_version; }
			bool altered(long currVersion, long newVersion) const;
			std::string drop() const;
			std::string create(long newVersion) const;
			void alter(long currVersion, long newVersion, bool add, std::list<std::string>& program) const;
			void alter_drop(long currVersion, long newVersion, std::list<std::string>& program) const;
		};

		class View
		{
			std::string m_name;
			std::string m_select;
			long m_version;
		public:
			View(const std::string& name, const std::string& select, long version = 1) : m_name(name), m_select(select), m_version(version) {}
			long version() const { return m_version; }
			std::string drop() const;
			std::string create() const;
		};

		using TransferCallback = std::function<void(const ConnectionPtr& conn, long currVersion, long newVersion, std::list<std::string>& program)>;

		class SchemaDefinition
		{
			std::list<Table> m_tables;
			std::list<View> m_views;
			std::list<std::pair<TransferCallback, long>> m_transfers;
		public:
			Table& table(const std::string& name, long version = 1)
			{
				m_tables.emplace_back(name, version);
				return m_tables.back();
			}
			View& view(const std::string& name, const std::string& select, long version = 1)
			{
				m_views.push_back(View(name, select, version));
				return m_views.back();
			}

			void transfer(TransferCallback cb, long version)
			{
				m_transfers.emplace_back(cb, version);
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

			void drop(long currVersion, long newVersion, std::list<std::string>& program) const;
			void drop_columns(long currVersion, long newVersion, std::list<std::string>& program) const;
			void create(long currVersion, long newVersion, std::list<std::string>& program) const;
			void transfer(const ConnectionPtr& conn, long currVersion, long newVersion, std::list<std::string>& program) const;
			void alter_add(long currVersion, long newVersion, std::list<std::string>& program) const;
			void alter_drop(long currVersion, long newVersion, std::list<std::string>& program) const;

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
			bool downgrade();
			bool addUser(const char* login, const char* mail, const char* name, const char* family_name);
			bool removeUser(const char* login);
			bool changePasswd(const char* login, const char* passwd);
			bool getUsers(Users& users);
			bool force_schema_config();
			long version();
			bool version(long);
		};

		template <typename T>
		inline void errorMessage(const char* title, const T& ptr)
		{
			const char* msg = ptr->errorMessage();
			if (msg && *msg)
				fprintf(stderr, "%s: %s\n", title, msg);
		}
	}
};

#endif //__DBCONN_SCHEMA_H__
