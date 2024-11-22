#pragma once
#include "Panel.h"

#include "AnimatorElements.h"

#include <Animacore/AnimatorController.h>

class Renderer;

namespace tool
{
	class AnimatorPanel : public Panel
	{
	public:
		AnimatorPanel(entt::dispatcher& dispatcher, Renderer* renderer);

		void RenderPanel(float deltaTime) override;
		PanelType GetType() override { return PanelType::Animator; }

	private:
		void selectEntity(const OnToolSelectEntity& event);
		void createContext(const OnToolAddedPanel& event);
		void showContextMenu();
		void destroyContext(const OnToolRemovePanel& event);
		void changeAnimator(const OnToolSelectFile& event);

		void generateNodes(const core::AnimatorController& controller);
		void printNode(Node* node);

		// left pane
		void showLeftPane(float paneWidth);
		void showParameters();

		void changeNodeName(Node* node, const std::string& newName);
		void refreshInspector();

		// play, stop
		void playScene(const OnToolPlayScene& event);
		void stopScene(const OnToolStopScene& event);

		Node* FindNode(ax::NodeEditor::NodeId id)
		{
			for (auto& node : m_Nodes)
				if (node.ID == id)
					return &node;

			return nullptr;
		}

		Node* FindNode(const std::string& name)
		{
			for (auto& node : m_Nodes)
				if (node.Name == name)
					return &node;

			return nullptr;
		}

		Link* FindLink(ax::NodeEditor::LinkId id)
		{
			for (auto& link : m_Links)
				if (link.ID == id)
					return &link;

			return nullptr;
		}

		void BuildNode(Node* node)
		{
			for (auto& input : node->Inputs)
			{
				input.Node = node;
				input.Kind = PinKind::Input;
			}

			for (auto& output : node->Outputs)
			{
				output.Node = node;
				output.Kind = PinKind::Output;
			}
		}

		Pin* FindPin(ax::NodeEditor::PinId id)
		{
			if (!id)
				return nullptr;

			for (auto& node : m_Nodes)
			{
				for (auto& pin : node.Inputs)
					if (pin.ID == id)
						return &pin;

				for (auto& pin : node.Outputs)
					if (pin.ID == id)
						return &pin;
			}

			return nullptr;
		}

		Node* SpawnStateNode(const std::string& name)
		{
			uint32_t id = entt::hashed_string(name.c_str()).value();

			m_Nodes.emplace_back(id, name.c_str());
			m_Nodes.back().Type = NodeType::Tree;
			
			// 이거 바꿔야함
			std::string inputName = name + " + In";
			std::string outputName = name + " + Out";

			uint32_t inputID = entt::hashed_string(inputName.c_str()).value();
			uint32_t outputID = entt::hashed_string(outputName.c_str()).value();

			m_Nodes.back().Inputs.emplace_back(inputID, inputName.c_str(), PinType::Flow);
			m_Nodes.back().Outputs.emplace_back(outputID, outputName.c_str(), PinType::Flow);

			BuildNodes( );

			return &m_Nodes.back();
		}

		void BuildNodes()
		{
			for (auto& node : m_Nodes)
				BuildNode(&node);
		}

		bool CanCreateLink(Pin* a, Pin* b)
		{
			if (!a || !b || a == b || a->Kind == b->Kind || a->Type != b->Type || a->Node == b->Node)
				return false;

			return true;
		}

		ImColor GetIconColor(PinType type)
		{
			switch (type)
			{
			default:
			case PinType::Flow:     return ImColor(255, 255, 255);
			case PinType::Bool:     return ImColor(220, 48, 48);
			case PinType::Int:      return ImColor(68, 201, 156);
			case PinType::Float:    return ImColor(147, 226, 74);
			case PinType::String:   return ImColor(124, 21, 153);
			case PinType::Object:   return ImColor(51, 150, 215);
			case PinType::Function: return ImColor(218, 0, 183);
			case PinType::Delegate: return ImColor(255, 48, 48);
			}
		}

	private:
		Renderer* _renderer = nullptr;
		std::vector<entt::entity> _selectedEntities;

		ax::NodeEditor::EditorContext* _context = nullptr;
		std::filesystem::path _selectControllerPath;
		std::string _selectControllerMetaPath;

		// 참조로 변경해야 할거같다..
		core::AnimatorController* _animatorController;

		core::Animator* _animator = nullptr;

		std::list<Node>    m_Nodes;
		std::vector<Link>    m_Links;

		std::string _searchQueryLower;

		bool _isDirty = false;

		bool _isPlaying = false;

		Pin* _newLinkPin = nullptr;

		friend class AnimatorDataPanel;
	};
}
