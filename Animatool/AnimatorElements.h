#pragma once

#include "../imgui_node_editor/imgui_node_editor.h"

namespace tool
{
	enum class PinType
	{
		Flow,
		Bool,
		Int,
		Float,
		String,
		Object,
		Function,
		Delegate,
	};

	enum class PinKind
	{
		Output,
		Input
	};

	enum class NodeType
	{
		Blueprint,
		Simple,
		Tree,
		Comment,
		Houdini
	};

	struct Node;

	struct Pin
	{
		ax::NodeEditor::PinId   ID;
		Node* Node;
		std::string Name;
		PinType     Type;
		PinKind     Kind;

		Pin(uint32_t id, const char* name, PinType type) :
			ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
		{
		}
		Pin(uint32_t id, const char* name, PinType type, PinKind kind) :
			ID(id), Node(nullptr), Name(name), Type(type), Kind(kind)
		{
		}
	};

	struct Node
	{
		ax::NodeEditor::NodeId ID;
		std::string Name;
		std::vector<Pin> Inputs;
		std::vector<Pin> Outputs;
		ImColor Color;
		NodeType Type;
		ImVec2 Size;

		std::string State;
		std::string SavedState;

		Node(uint32_t id, const char* name, ImColor color = ImColor(255, 255, 255)) :
			ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
		{
		}
	};

	struct Link
	{
		ax::NodeEditor::LinkId ID;

		std::string Name;

		ax::NodeEditor::PinId StartPinID;
		ax::NodeEditor::PinId EndPinID;

		ImColor Color;

		Link(ax::NodeEditor::LinkId id, ax::NodeEditor::PinId startPinId, ax::NodeEditor::PinId endPinId) :
			ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
		{
		}
	};
}