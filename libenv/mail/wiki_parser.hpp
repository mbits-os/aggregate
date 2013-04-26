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

#ifndef __WIKI_PARSER_HPP__
#define __WIKI_PARSER_HPP__

#include <vector>

namespace wiki { namespace parser {

	enum class TOKEN {
		BAD = -2,
		NOP,
		TEXT,
		ITALIC,
		BOLD, 
		BI,
		VAR_S,
		VAR_E,
		HREF_S,
		HREF_E,
		HREF_NS,
		HREF_SEG,
		TAG_S,
		TAG_E,
		TAG_CLOSED,
		BREAK
	};

	inline std::ostream& operator << (std::ostream& o, TOKEN tok)
	{
#define PRINT(x) case TOKEN::x: return o << #x;
		switch (tok)
		{
		PRINT(BAD);
		PRINT(NOP);
		PRINT(TEXT);
		PRINT(ITALIC);
		PRINT(BOLD);
		PRINT(BI);
		PRINT(VAR_S);
		PRINT(VAR_E);
		PRINT(HREF_S);
		PRINT(HREF_E);
		PRINT(HREF_NS);
		PRINT(HREF_SEG);
		PRINT(TAG_S);
		PRINT(TAG_E);
		PRINT(TAG_CLOSED);
		PRINT(BREAK);
		};
		return o << "unknown(" << (int)tok << ")";
#undef PRINT
	}

	struct Token
	{
		typedef std::string::const_iterator pointer;
		typedef std::pair<pointer, pointer> Arg;
		TOKEN type;
		Arg arg;

		Token(TOKEN tok, pointer start, pointer end): type(tok), arg(start, end) {}
	};

	inline std::ostream& operator << (std::ostream& o, const Token& tok)
	{
		if (tok.type == TOKEN::TEXT)
		{
			o << '"';
			std::for_each(tok.arg.first, tok.arg.second, [&o](char c) { o.put(c); });
			return o << '"';
		}

		o << tok.type;
		if (tok.arg.first != tok.arg.second)
		{
			o << " {";
			std::for_each(tok.arg.first, tok.arg.second, [&o](char c) { o.put(c); });
			o << "}";
		}

		return o;
	}

	namespace line
	{
		typedef std::vector<Token> Tokens;

		class Parser
		{
			typedef Token::pointer pointer;

			Tokens m_out;
			pointer m_prev;
			pointer m_cur;
			pointer m_end;

			bool inWiki;
			bool inHREF;
			bool in1stSEG;

			void push(TOKEN token, pointer start, pointer end) { m_out.emplace_back(token, start, end); }
			void push(TOKEN token) { m_out.emplace_back(token, m_end, m_end); }
			void text(size_t count); //push [m_prev, m_cur - count) as a TOKEN::TEXT
			void repeated(char c, size_t repeats, TOKEN tok);
			void boldOrItalics();
			void htmlTag();
		public:
			Parser(pointer start, pointer end)
				: m_prev(start)
				, m_cur(start)
				, m_end(end)
				, inWiki(true)
				, inHREF(false)
				, in1stSEG(false)
			{
			}
			Tokens parse();
		};
	}

	class Parser
	{
	public:
		enum class BLOCK
		{
			UNKNOWN,
			HEADER,
			PARA,
			QUOTE,
			PRE,
			UL,
			OL,
			ITEM,
			HR,
			SIGNATURE
		};

		struct Block;
		typedef std::vector<Block> Text;
		typedef Token::pointer pointer;

		struct Block
		{
			BLOCK type;
			int iArg;
			std::string sArg;
			line::Tokens tokens;
			Text items;
			Block(BLOCK type = BLOCK::UNKNOWN): type(type), iArg(0) {}
			void append(line::Tokens&& tokens);
		};

	private:
		Text m_out;
		Block m_cur;
		Text m_itemStack;

		void block(pointer start, pointer end);

		void header(pointer start, pointer end);
		void para(pointer start, pointer end);
		void quote(pointer start, pointer end);
		void pre(pointer start, pointer end);
		void item(pointer start, pointer end);
		void hr();
		void sig(pointer start, pointer end);
		void changeBlock(BLOCK newBlock);
		void reset(BLOCK type = BLOCK::UNKNOWN);
		void push();

		void parseLine(pointer line, pointer end);
	public:
		Parser();
		Text parse(const std::string& text);
	};

}}; // wiki::parser

#endif //__WIKI_PARSER_HPP__