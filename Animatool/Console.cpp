#include "pch.h"
#include "Console.h"

#include "ToolProcess.h"

// Console 클래스 정의
tool::Console::Console(entt::dispatcher& dispatcher)
	: Panel(dispatcher)
{
	auto sink = std::make_shared<ImGuiSink>(this);
	auto logger = std::make_shared<spdlog::logger>("console", sink);
	spdlog::register_logger(logger);
	spdlog::set_default_logger(logger);
	spdlog::set_level(spdlog::level::debug);
}

void tool::Console::RenderPanel(float deltaTime)
{
	if (ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::Button("Clear"))
			{
				std::lock_guard<std::mutex> lock(_mutex);
				_logs.clear();
			}
			if (ImGui::Checkbox("Show FPS", &_showFps))
			{
				_dispatcher->enqueue<OnToolShowFPS>(_showFps);
			}
			ImGui::EndMenuBar();
		}

		std::lock_guard<std::mutex> lock(_mutex);

		for (auto&& [type, msg] : _logs)
		{
			ImTextureID icon = nullptr;

			switch (type)
			{
			case MsgType::Error:
				icon = ToolProcess::icons["error"];
				break;
			case MsgType::Warn:
				icon = ToolProcess::icons["warn"];
				break;
			case MsgType::Info:
				icon = ToolProcess::icons["info"];
				break;
			case MsgType::Debug:
				icon = ToolProcess::icons["debug"];
				break;
			default:
				break;
			}

			if (icon)
			{
				ImGui::Image(icon, ImVec2(32, 32));
				ImGui::SameLine();
			}

			ImGui::TextUnformatted(msg.c_str());
			ImGui::Separator(); // 로그별 구분선 추가
		}

		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);
	}
	ImGui::End();
}

void tool::Console::AddLog(MsgType type, const std::string& message)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_logs.push_back({ type, message });
	if (_logs.size() > 1000)
	{
		_logs.pop_front();
	}
}

tool::Console::MsgType tool::Console::ConvertLevel(spdlog::level::level_enum type)
{
	switch (type)
	{
	case spdlog::level::err:
		return MsgType::Error;
	case spdlog::level::warn:
		return MsgType::Warn;
	case spdlog::level::info:
		return MsgType::Info;
	default:
		return MsgType::Debug;
	}
}

tool::ImGuiSink::ImGuiSink(Console* console)
	: _console(console)
{
	ImGuiSink::set_pattern("[%H:%M:%S]\n%v"); // 시간, 로그 레벨, 메시지 포맷 설정
}

void tool::ImGuiSink::log(const spdlog::details::log_msg& msg)
{
	spdlog::memory_buf_t formatted;
	_formatter->format(msg, formatted);

	_console->AddLog(_console->ConvertLevel(msg.level), fmt::to_string(formatted));
}

void tool::ImGuiSink::flush() {}

void tool::ImGuiSink::set_pattern(const std::string& pattern)
{
	_formatter = std::make_unique<spdlog::pattern_formatter>(pattern);
}

void tool::ImGuiSink::set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter)
{
	_formatter = std::move(sink_formatter);
}
