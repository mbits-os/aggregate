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
#include <crypt.hpp>
#include <model.hpp>
#include <algorithm>

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
					.field("login")
					.field("name")
					.field("email")
					.field("passphrase")
					.field("root_folder", "0", field_type::KEY)
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
					.field("ord", "0", field_type::INTEGER)
					;

				sd.table("feed")
					._id()
					.field("title")
					.field("site")
					.field("feed")
					.field("last_update", std::string(), field_type::TIME)
					.nullable("author")
					.nullable("authorLink")
					.nullable("etag")
					.nullable("last_modified")
					;

				sd.table("entry")
					._id()
					.refer("feed")
					.field("guid")
					.field("title", "")
					.nullable("url")
					.nullable("date", std::string(), field_type::TIME)
					.nullable("author")
					.nullable("authorLink")
					//only one of those can acutally be null:
					.nullable("description")
					.nullable("contents")
					;

				sd.table("enclosure")
					.refer("entry")
					.field("url")
					.field("mime")
					.field("length", std::string(), field_type::INTEGER)
					;

				sd.table("categories")
					.refer("entry")
					.field("cat")
					;

				sd.table("subscription")
					.refer("feed")
					.refer("folder")
					.field("ord", std::string(), field_type::INTEGER)
					;

				sd.table("state")
					.refer("user")
					.refer("entry")
					.field("type", std::string(), field_type::INTEGER) // 0 - unread; 1 - starred; ?2 - important?
					;

				sd.view("parents",
					"SELECT folder.user_id AS user_id, folder._id AS folder_id, folder.name AS name, parent.name AS parent, folder.ord AS ord "
					"FROM folder "
					"LEFT JOIN folder AS parent ON (parent._id = folder.parent) "
					"ORDER BY parent.name, folder.ord");

				sd.view("state_stats",
					"SELECT state.user_id AS user_id, entry.feed_id AS feed_id, state.type AS type, count(*) AS count "
					"FROM state "
					"LEFT JOIN entry ON (entry._id = state.entry_id) "
					"GROUP BY state.type, entry.feed_id, state.user_id");

				sd.view("ordered_stats",
					"SELECT state_stats.user_id AS user_id, subscription.folder_id AS folder_id, subscription.ord AS ord, feed._id AS feed_id, feed.title AS feed, type, count "
					"FROM subscription "
					"LEFT JOIN folder ON (subscription.folder_id = folder._id) "
					"LEFT JOIN feed ON (subscription.feed_id = feed._id) "
					"LEFT JOIN state_stats ON (state_stats.feed_id = feed._id AND state_stats.user_id = folder.user_id) "
					"ORDER BY folder_id, type, ord");

				sd.view("trending",
					"SELECT feed_id, count(*) AS popularity "
					"FROM subscription "
					"GROUP BY feed_id");

				sd.view("feed_update",
					"SELECT _id, feed, etag, last_modified, last_update, popularity "
					"FROM feed "
					"LEFT JOIN trending ON (trending.feed_id = feed._id) "
					"ORDER BY popularity DESC, last_update ASC");
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
			auto it = std::find_if(program.begin(), program.end(), [this](const std::string& sql) -> bool
			{
				bool ret = !m_conn->exec(sql.c_str());
				if (ret)
				{
					printf("%s\n", sql.c_str());
				}
				return ret;
			});
			if (it != program.end())
				return false;

			program = sd.create();
			it = std::find_if(program.begin(), program.end(), [this](const std::string& sql) -> bool
			{
				bool ret = !m_conn->exec(sql.c_str());
				if (ret)
				{
					printf("%s\n", sql.c_str());
				}
				return ret;
			});
			if (it != program.end())
				return false;

			return transaction.commit();
		}

		bool Schema::addUser(const char* login, const char* mail, const char* name)
		{
			char pass[13];
			Crypt::newSalt(pass);
			Crypt::password_t hash;
			Crypt::password(pass, hash);

			db::StatementPtr select = m_conn->prepare("SELECT count(*) FROM user WHERE email=? OR login=?");
			if (!select.get())
				return false;

			db::StatementPtr insert = m_conn->prepare("INSERT INTO user (login, name, email, passphrase) VALUES (?, ?, ?, ?)");
			if (!insert.get())
				return false;

			db::StatementPtr query = m_conn->prepare("SELECT _id FROM user WHERE login=?");
			if (!query.get())
				return false;

			db::StatementPtr insert_root = m_conn->prepare("INSERT INTO folder (user_id, name) VALUES (?, \"root\")");
			if (!insert_root.get())
				return false;

			db::StatementPtr update = m_conn->prepare("UPDATE user SET root_folder=(SELECT _id FROM folder WHERE user_id=? AND parent=0) WHERE _id=?");
			if (!update.get())
				return false;

			if (!select->bind(0, mail)) return false;
			if (!select->bind(1, login)) return false;

			if (!insert->bind(0, login)) return false;
			if (!insert->bind(1, name)) return false;
			if (!insert->bind(2, mail)) return false;
			if (!insert->bind(3, hash)) return false;

			if (!query->bind(0, login)) return false;

			db::Transaction transaction(m_conn);
			if (!transaction.begin())
				return false;

			CursorPtr c = select->query();
			if (!c.get()) return false;
			if (!c->next()) return false;
			long count = c->getLong(0);
			if (count != 0)
			{
				long long id = -1;
				fprintf(stderr, "error: user already exists:\n");
				select = m_conn->prepare("SELECT * FROM user WHERE login=?");
				if (select.get())
				{
					select->bind(0, login);
					c = select->query();
					if (c.get() && c->next())
					{
						id = c->getLongLong(0);
						const char* login = c->getText(1);
						const char* name = c->getText(2);
						const char* mail = c->getText(3);
						bool is_admin = c->getInt(4) != 0;
						fprintf(stderr, "    #%llu: %s / %s <%s> %s\n",
							id, login, name, mail, is_admin ? "(admin)" : ""
							);
					}
				}
				select = m_conn->prepare("SELECT * FROM user WHERE email=?");
				if (select.get())
				{
					select->bind(0, mail);
					c = select->query();
					if (c.get() && c->next() && (id == -1 || id != c->getLongLong(0)))
					{
						id = c->getLongLong(0);
						const char* login = c->getText(1);
						const char* name = c->getText(2);
						const char* mail = c->getText(3);
						bool is_admin = c->getInt(4) != 0;
						fprintf(stderr, "    #%llu: %s / %s <%s> %s\n",
							id, login, name, mail, is_admin ? "(admin)" : ""
							);
					}
				}

				return false;
			}
			if (!insert->execute()) return false;

			long long _id = -1;
			c = query->query();
			if (c && c->next())
				_id = c->getLongLong(0);
			if (_id == -1)
				return false;

			if (!insert_root->bind(0, _id)) return false;
			if (!update->bind(0, _id)) return false;
			if (!update->bind(1, _id)) return false;

			if (!insert_root->execute()) return false;
			if (!update->execute()) return false;

			printf("pass: %s\n", pass);
			return transaction.commit();
		}

		bool Schema::removeUser(const char* mail)
		{
			db::StatementPtr select = m_conn->prepare("SELECT count(*) FROM user WHERE email=?");
			if (!select.get())
				return false;

			db::StatementPtr del = m_conn->prepare("DELETE FROM user WHERE email=?");
			if (!del.get())
				return false;

			if (!select->bind(0, mail)) return false;
			if (!del->bind(0, mail)) return false;

			db::Transaction transaction(m_conn);
			if (!transaction.begin())
				return false;

			CursorPtr c = select->query();
			if (!c.get()) return false;
			if (!c->next()) return false;
			long count = c->getLong(0);
			if (count == 0)
			{
				fprintf(stderr, "error: no such user\n", mail);
				return false;
			}
			if (!del->execute()) return false;

			return transaction.commit();
		}

		bool Schema::changePasswd(const char* mail, const char* passwd)
		{
			Crypt::password_t hash;
			Crypt::password(passwd, hash);

			db::StatementPtr update = m_conn->prepare("UPDATE user SET passphrase=? WHERE email=?");
			if (!update.get())
				return false;

			if (!update->bind(0, hash)) return false;
			if (!update->bind(1, mail)) return false;

			return update->execute();
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