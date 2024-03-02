
#include "editor/logoutput.hpp"

#include "editor/editor.hpp"

#include "e2/ui/uicontext.hpp"

//#include "imgui.h"
//#include "imgui_ext.h"
//#include "misc/cpp/imgui_stdlib.h"

e2::LogOutput::LogOutput(e2::Editor* ed)
	: m_editor(ed)
{

}

e2::LogOutput::~LogOutput()
{

}

void e2::LogOutput::update(e2::UIContext* ui, double seconds)
{
	e2::UIStyle& style = uiManager()->workingStyle();

	uint32_t newEntries = e2::Log::numEntries();
	bool haveNewEntries = (newEntries != m_lastNumEntries);
	m_lastNumEntries = newEntries;


	static e2::Name id_VFlex("Log_VFlex");
	static e2::Name id_StackH("Log_StackH");
	static e2::Name id_AutoScroll("Log_Autoscroll");
	static e2::Name id_ClearLog("Log_Clear");
	static e2::Name id_Log("Log");
	float VFlex[] = { 24.0f * style.scale, 0.0f};
	ui->beginFlexV(id_VFlex, VFlex, 2);

		ui->beginStackH(id_StackH);

			ui->checkbox(id_AutoScroll, m_autoScroll, "Autoscroll");

			if (ui->button(id_ClearLog, "Clear"))
			{
				e2::Log::clear();
			}

		ui->endStackH();

		//ui->label("assetBrowser", "[Log Output]", 12, UITextAlign::Middle);
		ui->log(id_Log, m_scrollOffset, m_autoScroll && haveNewEntries);
	ui->endFlexV();
	/*if (ImGui::Begin("Asset Browser"))
	{
		ImGui::Text("Path: /%s", m_path.c_str());
		if (ImGui::BeginChild("##AssetContent"))
		{

		}
		ImGui::EndChild();

	}
	ImGui::End();*/
}

e2::Editor* e2::LogOutput::editor()
{
	return m_editor;
}

e2::Engine* e2::LogOutput::engine()
{
	return m_editor->engine();
}
