
#pragma once 

#include <e2/export.hpp>
#include <e2/context.hpp>

namespace e2
{

	class Editor;

	class EditorContext : public virtual e2::Context
	{
	public:
		virtual ~EditorContext();

		virtual e2::Editor* editor() = 0;


	};

}