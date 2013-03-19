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

#include "schema.h"
#include "crypt.hpp"
#include "model.h"

namespace db
{
	namespace model
	{
		struct SDBuilder
		{
			SchemaDefinition sd;
			SDBuilder()
			{
				sd.table("user")
					._id()
					.field("name")
					.field("email")
					.field("passphrase")
					.field("is_admin", "0", field_type::BOOLEAN)
					;

				//hash is built from email and new salt
				//hash goes into cookie, seed is saved for later
				sd.table("session")
					.text_id("hash")
					.field("seed")
					.refer("user")
					.field("set_on", std::string(), field_type::TIME)
					;

				sd.table("folder")
					._id()
					.refer("user")
					.field("name")
					.field("parent", "0", field_type::KEY)
					;

				sd.table("feed")
					._id()
					.field("title")
					.field("site")
					.field("feed")
					.field("last_update")
					;

				sd.table("entry")
					._id()
					.refer("feed")
					.field("guid")
					.field("title", "")
					.nullable("url")
					.nullable("date")
					.nullable("author")
					.field("contents")
					;

				sd.table("subscription")
					.refer("user")
					.refer("feed")
					.refer("folder")
					;

				sd.table("unread")
					.refer("user")
					.refer("entry")
					;

				sd.table("starred")
					.refer("user")
					.refer("entry")
					;
			}
		};

		SchemaDefinition& SchemaDefinition::schema()
		{
			static SDBuilder builder;
			return builder.sd;
		}

		bool Schema::install()
		{
			printf("dbtool: installing the schema\n");
			SchemaDefinition& sd = SchemaDefinition::schema();
			std::list<std::string> program;
			std::list<std::string>::const_iterator _cur, _end;

			db::Transaction transaction(m_conn);
			if (!transaction.begin())
				return false;

			program = sd.drop();
			for (_cur = program.begin(), _end = program.end(); _cur != _end; ++_cur)
			{
				//printf("%s\n", _cur->c_str());
				if (!m_conn->exec(_cur->c_str()))
					return false;
			}

			program = sd.create();
			for (_cur = program.begin(), _end = program.end(); _cur != _end; ++_cur)
			{
				//printf("%s\n", _cur->c_str());
				if (!m_conn->exec(_cur->c_str()))
					return false;
			}

			return transaction.commit();
		}

		bool Schema::addUser(const char* mail, const char* name)
		{
			char pass[13];
			crypt::newSalt(pass);
			crypt::Password hash;
			crypt::password(pass, hash);

			db::StatementPtr select = m_conn->prepare("SELECT count(*) FROM user WHERE email=?");
			if (!select.get())
				return false;

			db::StatementPtr insert = m_conn->prepare("INSERT INTO user (name, email, passphrase) VALUES (?, ?, ?)");
			if (!insert.get())
				return false;

			if (!select->bind(0, mail)) return false;
			if (!insert->bind(0, name)) return false;
			if (!insert->bind(1, mail)) return false;
			if (!insert->bind(2, hash)) return false;

			db::Transaction transaction(m_conn);
			if (!transaction.begin())
				return false;

			CursorPtr c = select->query();
			if (!c.get()) return false;
			if (!c->next()) return false;
			long count = c->getLong(0);
			if (count != 0)
			{
				fprintf(stderr, "error: user %s already exists\n", mail);
				return false;
			}
			if (!insert->execute()) return false;

			printf("pass: %s\n", pass);
			return transaction.commit();
		}

		bool Schema::removeUser(const char* mail)
		{
			return true;
		}

		std::string Field::repr() const
		{
			std::string sql = m_name + " ";
			switch (m_fld_type)
			{
			case field_type::INTEGER: sql += "INTEGER"; break;
			case field_type::KEY: sql += "BIGINT"; break;
			case field_type::TEXT: sql += "TEXT CHARACTER SET utf8"; break;
			case field_type::TEXT_KEY: sql += "VARCHAR(100) CHARACTER SET utf8"; break;
			case field_type::BOOLEAN: sql += "BOOLEAN"; break;
			case field_type::TIME: sql += "TIMESTAMP"; break;
			}

			if (m_attributes & att::NOTNULL)
				sql += " NOT NULL";
			if (m_attributes & att::AUTOINCREMENT)
				sql += " AUTO_INCREMENT";
			if (m_attributes & att::UNIQUE)
				sql += " UNIQUE";
			if (m_attributes & att::KEY)
				sql += " PRIMARY KEY";

			if (!m_ref.empty())
				if (m_attributes & att::REFERENCES)
				{
#if 0
					sql += " REFERENCES " + m_ref + " (_id)";
					if (m_attributes & att::DELETE_RESTRICT)
						sql += " ON DELETE RESTRICT";
					if (m_attributes & att::UPDATE_RESTRICT)
						sql += " ON UPDATE RESTRICT";
					if (m_attributes & att::DELETE_CASCADE)
						sql += " ON DELETE CASCADE";
					if (m_attributes & att::UPDATE_CASCADE)
						sql += " ON UPDATE CASCADE";
					if (m_attributes & att::DELETE_NULLIFY)
						sql += " ON DELETE SET NULL";
					if (m_attributes & att::UPDATE_NULLIFY)
						sql += " ON UPDATE SET NULL";
#endif
				}
				else if (m_attributes & att::DEFAULT)
				{
					sql += " DEFAULT ";
					if (m_fld_type == field_type::TEXT)
						sql += "\"" + m_ref + "\"";
					else
						sql += m_ref;
				}

			return sql;
		}

		void Field::constraints(std::list<std::string>& cos) const
		{
			if (!m_ref.empty() && m_attributes & att::REFERENCES)
			{
				std::string sql = "FOREIGN KEY (" + m_name + ") REFERENCES " + m_ref + " (_id)";
				if (m_attributes & att::DELETE_RESTRICT)
					sql += " ON DELETE RESTRICT";
				if (m_attributes & att::UPDATE_RESTRICT)
					sql += " ON UPDATE RESTRICT";
				if (m_attributes & att::DELETE_CASCADE)
					sql += " ON DELETE CASCADE";
				if (m_attributes & att::UPDATE_CASCADE)
					sql += " ON UPDATE CASCADE";
				if (m_attributes & att::DELETE_NULLIFY)
					sql += " ON DELETE SET NULL";
				if (m_attributes & att::UPDATE_NULLIFY)
					sql += " ON UPDATE SET NULL";
				//cos.push_back("INDEX (" + m_name + ")");
				cos.push_back(sql);
			}
		}

		std::string Table::drop() const
		{
			return "DROP TABLE IF EXISTS " + m_name;
		}

		std::string Table::create() const
		{
			std::string sql = "CREATE TABLE " + m_name + " (\n  ";
			bool first = true;
			for (auto _cur = m_fields.begin(); _cur != m_fields.end(); ++_cur)
			{
				if (first) first = false;
				else sql += ",\n  ";
				sql += _cur->repr();
			}
			std::list<std::string> cos;
			for (auto _cur = m_fields.begin(); _cur != m_fields.end(); ++_cur)
				_cur->constraints(cos);
			for (auto _cur = cos.begin(); _cur != cos.end(); ++_cur)
			{
				if (first) first = false;
				else sql += ",\n  ";
				sql += *_cur;
			}
			return sql += "\n)";
		}

		std::string View::drop() const
		{
			return "DROP VIEW IF EXISTS " + m_name;
		}

		std::string View::create() const
		{
			return "CREATE VIEW " + m_name + " AS (" + m_select + ")";
		}

	}
}