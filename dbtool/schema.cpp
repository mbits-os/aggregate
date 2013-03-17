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

namespace db
{
	namespace model
	{
		struct SDBuilder
		{
			SchemaDefinition sd;
			SDBuilder()
			{
				sd.table("users")
					._id()
					.field("name")
					.field("email");

				sd.table("folders")
					._id()
					.refer("users")
					.field("name")
					.field("parent", "0", field_type::KEY);

				sd.table("feeds")
					._id()
					.field("title")
					.field("site")
					.field("feed")
					.field("last_update")
					;

				sd.table("entry")
					._id()
					.refer("feeds")
					.field("guid")
					.field("title", "")
					.nullable("url")
					.nullable("date")
					.nullable("author")
					.field("contents");

				sd.table("subscriptions")
					.refer("users")
					.refer("feeds")
					.refer("folders");

				sd.table("unread")
					.refer("users")
					.refer("entry");

				sd.table("starred")
					.refer("users")
					.refer("entry");
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

			return true;
		}

		std::string Field::repr() const
		{
			std::string sql = m_name + " ";
			switch (m_fld_type)
			{
			case field_type::INTEGER: sql += "INTEGER"; break;
			case field_type::KEY: sql += "BIGINT UNSIGNED"; break;
			case field_type::TEXT: sql += "TEXT CHARACTER SET utf8"; break;
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
			std::string sql = "CREATE TABLE " + m_name + " (\n";
			bool first = true;
			for (auto _cur = m_fields.begin(); _cur != m_fields.end(); ++_cur)
			{
				if (first) first = false;
				else sql += ",\n";
				sql += _cur->repr();
			}
			std::list<std::string> cos;
			for (auto _cur = m_fields.begin(); _cur != m_fields.end(); ++_cur)
				_cur->constraints(cos);
			for (auto _cur = cos.begin(); _cur != cos.end(); ++_cur)
			{
				if (first) first = false;
				else sql += ",\n";
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