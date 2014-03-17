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
#include "schema_builder.hpp"
#include <crypt.hpp>

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
		static inline bool install(const ConnectionPtr& conn, long currVersion, long newVersion, const SchemaDefinition& sd)
		{
			std::list<std::string> program;

			sd.drop(currVersion, newVersion, program);
			sd.create(currVersion, newVersion, program);

#if 0 // DRY-RUN
			printf("\n-- ########################################################\n");
			printf("-- ## CHANGE FROM %d TO %d\n", currVersion, newVersion);
			printf("-- ########################################################\n\n");
			for (auto&& sql : program)
				printf("%s;\n", sql.c_str());
#else
			for (auto&& sql : program)
			{
				if (!conn->exec(sql.c_str()))
				{
					const char* error = conn->errorMessage();
					if (error && *error)
						printf("[SQL] error %d: %s\n", conn->errorCode(), error);
					else
						printf("[SQL] error %d\n", conn->errorCode());

					printf("%s;\n", sql.c_str());
					return false;
				}
			}
#endif

			return true;
		}

		static inline bool downgrade(const ConnectionPtr& conn, long currVersion, long newVersion, const SchemaDefinition& sd)
		{
			std::list<std::string> program;

			sd.drop(newVersion, currVersion, program);
			sd.drop_columns(newVersion, currVersion, program);

#if 0 // DRY-RUN
			printf("\n-- ########################################################\n");
			printf("-- ## CHANGE FROM %d TO %d\n", currVersion, newVersion);
			printf("-- ########################################################\n\n");
			for (auto&& sql : program)
				printf("%s;\n", sql.c_str());
#else
			for (auto&& sql : program)
			{
				if (!conn->exec(sql.c_str()))
				{
					const char* error = conn->errorMessage();
					if (error && *error)
						printf("[SQL] error %d: %s (ignored)\n", conn->errorCode(), error);
					else
						printf("[SQL] error %d (ignored)\n", conn->errorCode());

					printf("%s;\n", sql.c_str());
				}
			}
#endif

			return true;
		}

		SchemaDefinition& SchemaDefinition::schema()
		{
			static SDBuilder builder;
			return builder.sd;
		}

		bool Schema::install()
		{
			long currVersion = version();
			long newVersion = VERSION::CURRENT;
			if (newVersion < currVersion)
			{
				fprintf(stderr, "install: downgrading is not supported\n");
				return false;
			}
			if (newVersion == currVersion)
			{
				printf("install: schema already at version %d\n", newVersion);
				return true;
			}

			if (currVersion)
				printf("install: updating the schema from version %d to %d\n", currVersion, newVersion);
			else
				printf("install: installing the schema\n");

			SchemaDefinition& sd = SchemaDefinition::schema();

			db::Transaction transaction(m_conn);
			if (!transaction.begin())
				return false;

			if (!db::model::install(m_conn, currVersion, newVersion, sd))
				return false;

			if (!version(newVersion))
				return false;

			return transaction.commit();
		}

		bool Schema::downgrade()
		{
			long currVersion = version();
			long maxVersion = VERSION::CURRENT;

			if (maxVersion < currVersion)
			{
				fprintf(stderr, "downgrade: downgrading from unknown schema version (%d) is not supported\n", currVersion);
				return false;
			}

			if (!currVersion)
			{
				fprintf(stderr, "downgrade: schema dropping is not supported\n", currVersion);
				return false;
			}

			if (maxVersion == currVersion)
			{
				printf("downgrade: schema already at version %d\n", maxVersion);
				return true;
			}

			printf("downgrade: downgrading the schema from version %d to %d\n", maxVersion, currVersion);

			SchemaDefinition& sd = SchemaDefinition::schema();

			db::Transaction transaction(m_conn);
			if (!transaction.begin())
				return false;

			if (!db::model::downgrade(m_conn, maxVersion, currVersion, sd))
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

		bool Schema::force_schema_config()
		{
			SchemaDefinition sd;
			SDBuilder::schema_config(sd);
			return db::model::install(m_conn, 0, 1, sd);
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
					db::model::errorMessage("DB message", conn);
					return false;
				}

				if (!insert->bind(0, field))
				{
					db::model::errorMessage("Statement message", insert);
					return false;
				}
				if (!insert->bind(1, &value, sizeof(value)))
				{
					db::model::errorMessage("Statement message", insert);
					return false;
				}

				db::Transaction transaction(conn);
				if (!transaction.begin())
				{
					db::model::errorMessage("DB message", conn);
					return false;
				}

				if (!insert->execute())
				{
					db::model::errorMessage("Statement message", insert);
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
	}
}