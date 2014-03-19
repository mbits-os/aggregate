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
#include "settings.hpp"
#include "avatar.hpp"
#include "../auth/auth.hpp"

namespace FastCGI { namespace app { namespace reader {

	class Verbatim : public Control
	{
		std::string m_text;
	public:
		using Control::Control;

		void text(const std::string& value) { m_text = value; }
		void render(Request& request, BasicRenderer& renderer) override
		{
			renderSimple(request, renderer);
		}
		void renderSimple(Request& request, BasicRenderer& renderer) override
		{
			request << m_text;
		}
	};

	class Avatar : public Control
	{
	public:
		Avatar(const std::string& name, const std::string& label, const std::string& hint)
			: Control(name, label, hint)
		{
			setAttr("src", "/icon.png?s=96");
			setAttr("width", "96");
			setAttr("height", "96");
		}

		void getControlString(Request& request, BasicRenderer& renderer) override
		{
			renderer.getControlString(request, this, "img", m_hasError);
		}
	};

	class ProfileSettingsPageHandler : public SettingsPageHandler
	{
	public:
		DEBUG_NAME("Settings: Profile");

	protected:
		void prerender(const SessionPtr& session, Request& request, PageTranslation& tr) override
		{
			if (request.getVariable("close"))
				onPageDone(request);

			auto content = std::make_shared<settings::SectionForm>(settings::PAGE::PROFILE);
			request.setContent(content);

			std::string url = "/auth/change";
			auto here_uri = request.getParam("DOCUMENT_URI");
			if (here_uri)
				url += "?continue=" + url::encode(request.serverUri(here_uri, false));

			auto& signin = content->section(tr(lng::LNG_SETTINGS_PROFILE_SEC_SIGNIN));
			auto& name = content->section(tr(lng::LNG_SETTINGS_PROFILE_SEC_NAME));

			content->buttons().submit("submit", tr(lng::LNG_CMD_UPDATE));
			content->buttons().submit("close", tr(lng::LNG_CMD_CLOSE), ButtonType::Narrow);

			auto login = signin.text("login", tr(lng::LNG_LOGIN_USERNAME));
			auto pass = signin.text("password", tr(lng::LNG_LOGIN_PASSWORD), true, tr(lng::LNG_SETTINGS_PROFILE_PASSWORD_MAIL_HINT));
			auto email = signin.text("email", tr(lng::LNG_SETTINGS_PROFILE_EMAIL));
			signin.link("pass", url, tr(lng::LNG_SETTINGS_PROFILE_CHANGE_PASSWORD));

			Options opts;
			opts.add("0", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_CUSTOM))
				.add("1", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_ORIGINAL))
				.add("2", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_NAME))
				.add("3", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_FAMILY_NAME))
				.add("4", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_NFN))
				.add("5", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_FNN))
				.add("6", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_NCFN))
				.add("7", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME_FNCN));

			auto given_name = name.text("name", tr(lng::LNG_SETTINGS_PROFILE_NAME));
			auto family_name = name.text("family_name", tr(lng::LNG_SETTINGS_PROFILE_FAMILY_NAME));
			auto display_name = name.selection("display_name", tr(lng::LNG_SETTINGS_PROFILE_DISPLAY_NAME), opts);
			auto custom_name = name.text("custom", std::string());
			auto script = name.control<Verbatim>("script");

			const auto& avatars = avatar::Engines::engines();
			RadioGroupPtr engines;
			if (avatars.size() > 1) // if there is anything more installed, than the default engine
			{
				auto& avatar = content->section(tr(lng::LNG_SETTINGS_PROFILE_SEC_AVATAR));
				avatar.control<Avatar>("icon");
				engines = avatar.radio_group("avatar", tr(lng::LNG_SETTINGS_PROFILE_WHICH_AVATAR));
				auto script = avatar.control<Verbatim>("script");

				engines->radio("default", tr(lng::LNG_SETTINGS_PROFILE_AVATAR_NONE));
				for (auto&& engine : avatars)
				{
					if (engine.first == "default") continue;
					auto e = avatar::Engines::engine(engine);
					if (!e->name()) continue;
					engines->radio(engine.first, tr(lng::LNG_SETTINGS_PROFILE_USE_AVATAR_FROM, e->name(), e->homePage()));
				}

				script->text(R"(<script type='text/javascript'>
$(function ()
{
	var original_uri = $("#icon").attr("src");

	$("input[name=avatar]").change(function()
	{
		$("#icon").attr("src", original_uri + "&force_avatar=" + $(this).val());
	});
});
</script>
<style type="text/css">
#icon
{
	-webkit-box-shadow: 1px 1px 5px 0px rgba(0, 0, 0, 0.4);
	-moz-box-shadow: 1px 1px 5px 0px rgba(0, 0, 0, 0.4);
	box-shadow: 1px 1px 5px 0px rgba(0, 0, 0, 0.4);
	padding: 4px;
	border-radius: 27px;
}
</style>
)");
			}

			auto profile = session->profile();

			Strings data;
			data["login"] = profile->login();
			data["email"] = profile->email();
			data["name"] = profile->name();
			data["family_name"] = profile->familyName();
			data["custom"] = profile->displayName();

			auto avatar_engine = std::tolower(profile->avatarEngine());
			bool found = false;
			for (auto&& engine : avatars)
			{
				if (engine.first == avatar_engine)
				{
					found = true;
					break;
				}
			}

			if (!found)
				avatar_engine = "default";
			data["avatar"] = avatar_engine;

			content->bind(request, data);
			login->setAttr("disabled", "disabled");
			script->text(
				"<script type='text/javascript'>$(\"#display_name\").attr(\"custom\", \"" + escape(profile->displayName()) + "\");</script>\r\n"
				"<script type='text/javascript' src='" + request.getStaticResources() + "code/username.js'></script>\r\n"
				);


			if (!request.getVariable("posted"))
				return;

			Update update{ tr };

			update.update("email", email, profile->email(), lng::LNG_SETTINGS_PROFILE_ERROR_EMAIL_FILLED, [&]{
				if (!pass->hasUserData())
				{
					update.errors.push_back(tr(lng::LNG_SETTINGS_PROFILE_ERROR_INVALID_PASSWORD));
					email->setError();
					pass->setError();
					return false;
				}

				auto db = request.dbConn();
				auto user = UserInfo::fromSession(db, session);
				if (!user.passwordValid(pass->getData().c_str()))
				{
					update.errors.push_back(tr(lng::LNG_SETTINGS_PROFILE_ERROR_INVALID_PASSWORD));
					email->setError();
					pass->setError();
					return false;
				}

				auto tmp = UserInfo::fromDB(db, email->getData().c_str());
				if (tmp.m_id != 0)
				{
					update.errors.push_back(tr(lng::LNG_SETTINGS_PROFILE_ERROR_EMAIL_ALREADY_USED));
					email->setError();
					return false;
				}
				return true;
			});

			update.update("name", given_name, profile->name(), lng::LNG_SETTINGS_PROFILE_ERROR_NAME_REQUIRED);
			update.update("family_name", family_name, profile->familyName(), lng::LNG_SETTINGS_PROFILE_ERROR_FAMILY_NAME_REQUIRED);

			std::string real_display_name;
			update.update("display_name", display_name, "", lng::LNG_SETTINGS_PROFILE_ERROR_NO_OPTION, [&]() {
				uint32_t ndx = 0;
				if (!safe_atou(display_name->getData(), ndx))
				{
					update.errors.push_back(tr(lng::LNG_SETTINGS_PROFILE_ERROR_OPTION_OUTSIDE));
					display_name->setError();
					return false;
				}

				switch (ndx)
				{
				case 0:
					if (!update.update("dummy", custom_name, "", lng::LNG_SETTINGS_PROFILE_ERROR_CUSTOM_NEEDED))
					{
						display_name->setError();
						return false;
					}

					{
						auto it = update.data.find("dummy");
						if (it == update.data.end())
						{
							update.errors.push_back(tr(lng::LNG_SETTINGS_PROFILE_ERROR_CUSTOM_NEEDED));
							display_name->setError();
							custom_name->setError();
							return false;
						}

						real_display_name = it->second;
						update.data.erase(it);
					}
					break;

				case 1: real_display_name = profile->displayName(); break;
				case 2: real_display_name = given_name->getData(); break;
				case 3: real_display_name = family_name->getData(); break;
				case 4: real_display_name = given_name->getData() + " " + family_name->getData(); break;
				case 5: real_display_name = family_name->getData() + " " + given_name->getData(); break;
				case 6: real_display_name = given_name->getData() + ", " + family_name->getData(); break;
				case 7: real_display_name = family_name->getData() + ", " + given_name->getData(); break;

				default:
					update.errors.push_back(tr(lng::LNG_SETTINGS_PROFILE_ERROR_OPTION_OUTSIDE));
					display_name->setError();
					return false;
				}

				return real_display_name != profile->displayName();
			}, [&](const std::string&) { return real_display_name; });

			if (engines)
			{
				update.update("avatar", engines, profile->avatarEngine(), lng::LNG_SETTINGS_PROFILE_ERROR_AVATAR_MISSING, [&]{
					bool valid = avatars.find(engines->getData()) != avatars.end();
					if (!valid)
					{
						update.errors.push_back(tr(lng::LNG_SETTINGS_PROFILE_ERROR_UNKNOWN_AVATAR));
						engines->setError();
					}
					return valid;
				});
			}

			if (!update.errors.empty())
			{
				if (update.errors.size() == 1)
				{
					content->setError(update.errors.front());
				}
				else
				{
					std::ostringstream o;
					o << tr(lng::LNG_SETTINGS_PROFILE_ERROR_SOME_ISSUES) << "\r\n<ul>\r\n";
					for (auto&& error : update.errors)
						o << "  <li>" << error << "</li>\r\n";
					o << "</ul>\r\n";
					content->setError(o.str());
				}
			}
			else if (!update.data.empty())
			{
				profile->updateData(request.dbConn(), update.data);
				onPageDone(request);
			}
		}

		static bool safe_atou(const std::string& val, uint32_t& ret)
		{
			ret = 0;
			for (auto&& c : val)
			{
				switch (c)
				{
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					ret *= 10;
					ret += c - '0';
					break;
				default:
					return false;
				}
			}
			return true;
		}

		struct Update
		{
			std::map<std::string, std::string> data;
			std::vector<std::string> errors;
			PageTranslation& tr;
			Update(PageTranslation& tr) : tr(tr) {}

			template <typename T>
			bool update(const std::string& name, const std::shared_ptr<T>& ctrl, const std::string& defVal, lng::LNG err)
			{
				if (!ctrl->hasUserData())
				{
					errors.push_back(tr(err));
					ctrl->setError();
					return false;
				}
				if (ctrl->getData() != defVal)
				{
					data[name] = ctrl->getData();
				}
				return true;
			}

			template <typename T, typename Validator>
			bool update(const std::string& name, const std::shared_ptr<T>& ctrl, const std::string& defVal, lng::LNG err, Validator validate)
			{
				if (!ctrl->hasUserData())
				{
					errors.push_back(tr(err));
					ctrl->setError();
					return false;
				}
				if (ctrl->getData() != defVal)
				{
					if (validate())
						data[name] = ctrl->getData();
				}
				return true;
			}

			template <typename T, typename Validator, typename Filter>
			bool update(const std::string& name, const std::shared_ptr<T>& ctrl, const std::string& defVal, lng::LNG err, Validator validate, Filter filter)
			{
				if (!ctrl->hasUserData())
				{
					errors.push_back(tr(err));
					ctrl->setError();
					return false;
				}
				if (ctrl->getData() != defVal)
				{
					if (validate())
						data[name] = filter(ctrl->getData());
				}
				return true;
			}
		};

		static std::string escape(const std::string& s)
		{
			std::string out;
			for (auto&& c: s)
			{
				switch (c)
				{
				case '"': out.append("\\\""); break;
				case '\\': out.append("\\\\"); break;
				case '\r': out.append("\\\r"); break;
				case '\n': out.append("\\\n"); break;
				case '\t': out.append("\\\t"); break;
				default: out.push_back(c);
				}
			}
			return out;
		}
	};

}}} // FastCGI::app::reader

REGISTER_HANDLER("/settings/profile", FastCGI::app::reader::ProfileSettingsPageHandler);
