 
#pragma once

#include <e2/utils.hpp>

#include "game/gamecontext.hpp"

#include <nlohmann/json.hpp>

namespace e2
{

    using ScriptNodeId = uint64_t;

    class ScriptGraph;

    constexpr uint64_t maxNumOutputPins = 8;

    class ScriptNode : public e2::Object
    {
        ObjectDeclaration();
    public:
        ScriptNode() = default;
        ScriptNode(ScriptGraph* graph, ScriptNodeId nodeId);

        virtual void postConstruct(ScriptGraph* graph, ScriptNodeId nodeId);

        virtual ~ScriptNode();

        virtual void serialize(nlohmann::json& obj) = 0;
        virtual void deserialize(nlohmann::json& obj) = 0;

        virtual void onActivate() = 0;
        virtual void onUpdate(double seconds) = 0;
        virtual void onDeactivate() = 0;

        virtual bool isDone() = 0;
        virtual uint64_t resultPin() = 0;

        virtual uint64_t numOutputPins()
        {
            return 1;
        }

        virtual std::string const& outputPin(uint64_t index)
        {
            static const std::string name("output");
            return name;
        }

        inline ScriptNodeId id();

    protected:
        ScriptGraph* m_graph{};
        ScriptNodeId m_id;
    };

    struct ScriptConnectionKey
    {
        ScriptNodeId startNode;
        uint64_t startPinIndex;

    };


    inline bool operator== (ScriptConnectionKey const& lhs, ScriptConnectionKey const& rhs) noexcept
    {
        return lhs.startNode == rhs.startNode && lhs.startPinIndex == rhs.startPinIndex;
    }

}

template <>
struct std::hash<e2::ScriptConnectionKey>
{
    std::size_t operator()(const e2::ScriptConnectionKey& k) const
    {

        return ((std::hash<uint64_t>()(k.startNode)
            ^ (std::hash<uint64_t>()(k.startPinIndex) << 1)) >> 1);
    }
};

namespace e2 
{


    class ScriptGraph : public e2::GameContext
    {
    public:
        ScriptGraph(e2::GameContext* ctx, std::string const& path);

        virtual ~ScriptGraph();

        virtual e2::Game* game() override;

        ScriptNode* nodeById(ScriptNodeId id);

        ScriptNode* endNodeAtPin(ScriptNodeId startNode, uint64_t startPinIndex);

        ScriptNode* entryNode();

        e2::Name id()
        {
            return m_id;
        }

        bool valid()
        {
            return m_valid;
        }

    protected:
        e2::Game* m_game{};
        e2::Name m_id;
        bool m_valid{};
        std::unordered_map<ScriptNodeId, ScriptNode*> m_nodes;
        std::unordered_map<ScriptConnectionKey, ScriptNodeId> m_connections;
        ScriptNodeId m_entryNode;
    };

    class ScriptExecutionContext
    {
    public:
        ScriptExecutionContext(ScriptGraph* graph);

        void update(double seconds);

        bool isDone();

    protected:
        ScriptGraph* m_graph{};
        e2::ScriptNode* m_currentNode{};

    };





    struct DialogueLine
    {
        std::string text;
        uint64_t currentCharacterOffset{};
    };

    struct DialogueAnswer
    {
        std::string text;
    };

    constexpr uint64_t maxNumDialogueAnswers = 4;
    constexpr uint64_t maxNumDialogueLines = 5;


    /** @tags(dynamic) */
    class DialogueNode : public e2::ScriptNode
    {
        ObjectDeclaration();
    public:
        DialogueNode()=default;
        virtual ~DialogueNode();

        virtual void serialize(nlohmann::json& obj) override;
        virtual void deserialize(nlohmann::json& obj) override;

        virtual void onActivate() override;
        virtual void onUpdate(double seconds) override;
        virtual void onDeactivate() override;

        virtual bool isDone() override;
        virtual uint64_t resultPin() override;


        virtual uint64_t numOutputPins() override;

        virtual std::string const & outputPin(uint64_t index);


    protected:
        e2::StackVector<DialogueAnswer, e2::maxNumDialogueAnswers> m_answers;
        e2::StackVector<DialogueLine, e2::maxNumDialogueLines> m_lines;
        uint64_t m_currentLineOffset{};
        uint64_t m_selectedAnswer{};
        bool m_done{};
        double m_secondsSinceCharacterOffset{};
    };

}

#include "script.generated.hpp"