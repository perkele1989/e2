
#pragma once 

#include <e2/export.hpp>
#include <editor/editorcontext.hpp>

#include <string>

namespace e2
{
	class UIContext;

	class LogOutput : public EditorContext
	{
	public:
		LogOutput(e2::Editor* ed);
		virtual ~LogOutput();

		/** Draw IMGUI etc. */
		virtual void update(e2::UIContext* ui, double seconds);

		virtual e2::Editor* editor() override;

		virtual e2::Engine* engine() override;

		inline std::string const& currentPath() const
		{
			return m_path;
		}

		void refresh();

	protected:


		e2::Editor* m_editor{};
		std::string m_path;

		float m_scrollOffset{};
		bool m_autoScroll{true};
		uint32_t m_lastNumEntries{};
	};

}