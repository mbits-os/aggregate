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
				sd.table("schema_config")
					.field("name", std::string(), FIELD_TYPE::TEXT, att::KEY | att::NOTNULL)
					.field("value", std::string(), FIELD_TYPE::BLOB, att::NONE)
					;

				sd.table("user")
					._id()
					.field("login")
					.field("name")
					.field("email")
					.field("passphrase")
					.field("root_folder", "0", FIELD_TYPE::KEY)
					.field("is_admin", "0", FIELD_TYPE::BOOLEAN)
					;

				//hash is built from email and new salt
				//hash goes into cookie, seed is saved for later
				sd.table("session")
					.text_id("hash")
					.field("seed")
					.refer("user")
					.field("set_on", std::string(), FIELD_TYPE::TIME)
					;

				sd.table("folder")
					._id()
					.refer("user")
					.field("name")
					.field("parent", "0", FIELD_TYPE::KEY)
					.field("ord", "0", FIELD_TYPE::INTEGER)
					;

				sd.table("feed")
					._id()
					.field("title")
					.field("site")
					.field("feed")
					.field("last_update", std::string(), FIELD_TYPE::TIME)
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
					.nullable("date", std::string(), FIELD_TYPE::TIME)
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
					.field("length", std::string(), FIELD_TYPE::INTEGER)
					;

				sd.table("categories")
					.refer("entry")
					.field("cat")
					;

				sd.table("subscription")
					.refer("feed")
					.refer("folder")
					.field("ord", std::string(), FIELD_TYPE::INTEGER)
					;

				sd.table("state")
					.refer("user")
					.refer("entry")
					.field("type", std::string(), FIELD_TYPE::INTEGER) // 0 - unread; 1 - starred; ?2 - important?
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
					"SELECT state_stats.user_id AS user_id, subscription.folder_id AS folder_id, subscription.ord AS ord, feed._id AS feed_id, feed.title AS feed, feed.site AS feed_url, type, count "
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
					"SELECT _id, title, feed, etag, last_modified, last_update, popularity "
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

		bool Schema::getUsers(Users& users)
		{
			db::StatementPtr select = m_conn->prepare("SELECT login, name, email FROM user");
			if (!select.get())
				return false;

			db::Transaction transaction(m_conn);
			if (!transaction.begin())
				return false;

			CursorPtr c = select->query();
			if (!c.get()) return false;
			while (c->next())
			{
				User usr;
				usr.login = c->getText(0);
				usr.name = c->getText(1);
				usr.email = c->getText(2);
				users.push_back(usr);
			}

			return transaction.commit();
		}

		namespace
		{
			template <typename T>
			T getConfig(const db::ConnectionPtr& conn, const char* field, T defValue = T())
			{
				auto select = conn->prepare("SELECT value FROM schema_config WHERE name=?");
				if (!select.get())
					return defValue;

				if (!select->bind(0, field))
					return defValue;

				db::Transaction transaction(conn);
				if (!transaction.begin())
					return defValue;

				auto c = select->query();
				if (!c.get() || !c->next())
					return defValue;

				if (c->isNull(0))
					return defValue;

				auto ptr = c->getBlob(0);
				auto len = c->getBlobSize(0);
				if (len != sizeof(T))
					return defValue;

				T out{ *(const T*)ptr };

				transaction.commit();

				return out;
			}
			template <typename T>
			bool setConfig(const db::ConnectionPtr& conn, const char* field, const T& value)
			{
				auto insert = conn->prepare("INSERT INTO schema_config (name, value) VALUES (?, ?) ON DUPLICATE KEY UPDATE value=VALUES(value)");
				if (!insert.get())
				{
					fprintf(stderr, "DB message: %s\n", conn->errorMessage());
					return false;
				}

				if (!insert->bind(0, field))
				{
					fprintf(stderr, "Statement message: %s\n", insert->errorMessage());
					return false;
				}
				if (!insert->bind(1, &value, sizeof(value)))
				{
					fprintf(stderr, "Statement message: %s\n", insert->errorMessage());
					return false;
				}

				db::Transaction transaction(conn);
				if (!transaction.begin())
				{
					fprintf(stderr, "DB message: %s\n", conn->errorMessage());
					return false;
				}

				if (!insert->execute())
				{
					fprintf(stderr, "Statement message: %s\n", insert->errorMessage());
					return false;
				}

				return transaction.commit();
			}
		}

		long Schema::version()
		{
			return getConfig<long>(m_conn, "version");
		}

		bool Schema::version(long v)
		{
			return setConfig<long>(m_conn, "version", v);
		}

		std::string Field::repr() const
		{
			std::string sql = m_name + " ";
			switch (m_fld_type)
			{
			case FIELD_TYPE::INTEGER: sql += "INTEGER"; break;
			case FIELD_TYPE::KEY: sql += "BIGINT"; break;
			case FIELD_TYPE::TEXT: sql += "TEXT CHARACTER SET utf8"; break;
			case FIELD_TYPE::TEXT_KEY: sql += "VARCHAR(100) CHARACTER SET utf8"; break;
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
					if (m_fld_type == FIELD_TYPE::TEXT)
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