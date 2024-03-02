
#pragma once 

#include <e2/export.hpp>

#include <cstdint>

namespace esl
{

	enum class TokenType : uint32_t
	{

	};

	struct Token
	{

	};

	class E2_API Lexer
	{
	public:
		Lexer();
		virtual ~Lexer();

	protected:
	};
}
