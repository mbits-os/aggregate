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

#include "schema.hpp"
#include <algorithm>

#ifdef __CYGWIN__
namespace std
{
	inline std::string to_string(int i)
	{
		char buffer[64];
		sprintf(buffer, "%d", i);
		return buffer;
	}
}
#endif

namespace db
{
	namespace model
	{
		std::string Field::repr(bool alter) const
		{
			std::string sql = m_name + " ";
			switch (m_fld_type)
			{
			case FIELD_TYPE::INTEGER: sql += "INTEGER"; break;
			case FIELD_TYPE::KEY: sql += "BIGINT"; break;
			case FIELD_TYPE::TEXT: sql += "TEXT CHARACTER SET utf8"; break;
			case FIELD_TYPE::TEXT_KEY:
				if (m_length > 0)
					sql += "VARCHAR(" + std::to_string(m_length) + ") CHARACTER SET utf8";
				else
					sql += "VARCHAR(100) CHARACTER SET utf8";
				break;
			case FIELD_TYPE::BOOLEAN: sql += "BOOLEAN"; break;
			case FIELD_TYPE::TIME: sql += "TIMESTAMP"; break;
			case FIELD_TYPE::BLOB: sql += "BLOB"; break;
			}

			if (m_attributes & att::NOTNULL)
				sql += " NOT NULL";
			if (m_attributes & att::AUTOINCREMENT)
				sql += " AUTO_INCREMENT";
			if (m_attributes & att::UNIQUE)
				sql += " UNIQUE";
			if (m_attributes & att::KEY)
				sql += " PRIMARY KEY";

			if (alter || (!m_ref.empty() && m_attributes & att::DEFAULT))
			{
				sql += " DEFAULT ";
				if (m_ref.empty())
					sql += "NULL"; // in case of alter
				else if (m_fld_type == FIELD_TYPE::TEXT)
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

		bool Table::altered(long currVersion, long newVersion) const
		{
			for (auto&& field : m_fields)
			{
				if (field.altered(currVersion, newVersion))
					return true;
			}
			return false;
		}

		std::string Table::drop() const
		{
			return "DROP TABLE IF EXISTS " + m_name;
		}

		std::string Table::create(long newVersion) const
		{
			std::string sql = "CREATE TABLE " + m_name + " (\n  ";
			bool first = true;

			std::list<std::string> cos;
			for (auto&& field : m_fields)
			{
				if (field.version() > newVersion)
					continue;

				if (first) first = false;
				else sql += ",\n  ";
				sql += field.repr();
				field.constraints(cos);
			}

			for (auto&& con: cos)
			{
				if (first) first = false;
				else sql += ",\n  ";
				sql += con;
			}
			return sql += "\n)";
		}

		void Table::alter(long currVersion, long newVersion, std::list<std::string>& program) const
		{
			std::list<std::string> cos;

			for (auto&& field : m_fields)
			{
				if (!field.altered(currVersion, newVersion))
					continue;

				std::string sql = "ALTER TABLE ";
				sql.append(m_name);
				sql.append(" ADD COLUMN ");
				sql.append(field.repr(true));
				program.push_back(sql);

				field.constraints(cos);
			}

			for (auto&& con : cos)
			{
				std::string sql = "ALTER TABLE ";
				sql.append(m_name);
				sql.append(" ADD CONSTRAINT ");
				sql.append(con);
				program.push_back(sql);
			}
		}

		void Table::alter_drop(long currVersion, long newVersion, std::list<std::string>& program) const
		{
			for (auto&& field : m_fields)
			{
				if (!field.altered(currVersion, newVersion))
					continue;

				std::string sql = "ALTER TABLE ";
				sql.append(m_name);
				sql.append(" DROP COLUMN ");
				sql.append(field.name());
				program.push_back(sql);
			}
		}

		std::string View::drop() const
		{
			return "DROP VIEW IF EXISTS " + m_name;
		}

		std::string View::create() const
		{
			return "CREATE VIEW " + m_name + " AS (" + m_select + ")";
		}

		void SchemaDefinition::drop(long currVersion, long newVersion, std::list<std::string>& program) const
		{
			for (auto && v : reverse(m_views))
			{
				if (version_valid(currVersion, newVersion, v.version()))
					program.push_back(v.drop());
			}

			for (auto && t : reverse(m_tables))
			{
				if (version_valid(currVersion, newVersion, t.version()))
					program.push_back(t.drop());
			}
		}

		void SchemaDefinition::drop_columns(long currVersion, long newVersion, std::list<std::string>& program) const
		{
			for (auto && t : reverse(m_tables))
			{
				if (t.version() <= currVersion && t.altered(currVersion, newVersion))
					t.alter_drop(currVersion, newVersion, program);
			}
		}

		void SchemaDefinition::create(long currVersion, long newVersion, std::list<std::string>& program) const
		{
			for (auto && t : m_tables)
			{
				if (version_valid(currVersion, newVersion, t.version()))
				{
					program.push_back(t.create(newVersion));
				}
				else if (t.version() <= currVersion && t.altered(currVersion, newVersion))
				{
					t.alter(currVersion, newVersion, program);
				}
			}

			for (auto && v : m_views)
			{
				if (version_valid(currVersion, newVersion, v.version()))
					program.push_back(v.create());
			}
		}

	}
}