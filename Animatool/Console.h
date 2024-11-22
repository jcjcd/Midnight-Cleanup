#pragma once
#include "Panel.h"

#include <Animacore/CoreSystemEvents.h>

namespace tool
{
	class Console : public tool::Panel
	{
        using MsgType = core::OnThrow::Type;

    public:
        explicit Console(entt::dispatcher& dispatcher);

        void RenderPanel(float deltaTime) override;
        PanelType GetType() override { return PanelType::Console; }

        void AddLog(MsgType type, const std::string& message);
        MsgType ConvertLevel(spdlog::level::level_enum type);

    private:
        std::deque<std::pair<MsgType, std::string>> _logs;
        std::mutex _mutex;

        bool _showFps = false;
	};

    class ImGuiSink : public spdlog::sinks::sink
    {
    public:
        ImGuiSink(Console* console);

        void log(const spdlog::details::log_msg& msg) override;
        void flush() override;
        void set_pattern(const std::string& pattern) override;
        void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override;

    private:
        Console* _console;
        std::unique_ptr<spdlog::formatter> _formatter;
    };

}
