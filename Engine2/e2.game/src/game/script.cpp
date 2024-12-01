
#include "game/script.hpp"
#include "game/game.hpp"
#include <e2/game/gamesession.hpp>

e2::ScriptNode::ScriptNode(ScriptGraph* graph, ScriptNodeId nodeId)
{
    postConstruct(graph, nodeId);
}

void e2::ScriptNode::postConstruct(ScriptGraph* graph, ScriptNodeId nodeId)
{
    m_graph = graph;
    m_id = nodeId;
}

e2::ScriptNode::~ScriptNode()
{
}

e2::ScriptNodeId e2::ScriptNode::id()
{
    return m_id;
}

e2::ScriptGraph::ScriptGraph(e2::GameContext* ctx, std::string const& path)
    : m_game(ctx->game())
{
    std::ifstream graphFile(path);
    nlohmann::json graph;
    try
    {
        graph = nlohmann::json::parse(graphFile);
    }
    catch (nlohmann::json::parse_error& e)
    {
        LogError("failed to load script graph from \"{}\", json parse error: {}", path, e.what());
        return;
    }

    if (!graph.contains("id"))
    {
        LogError("failed to load script graph from \"{}\", missing id", path);
        return;
    }

    m_id = graph.at("id").template get<std::string>();

    if (!graph.contains("entryNode"))
    {
        LogError("failed to load script graph from \"{}\", missing entry node", path);
        return;
    }

    if (!graph.contains("nodes"))
    {
        LogError("failed to load script graph from \"{}\", missing nodes array", path);
        return;
    }

    if (!graph.contains("connections"))
    {
        LogError("failed to load script graph from \"{}\", missing connections array", path);
        return;
    }

    m_entryNode = graph.at("entryNode").template get<uint64_t>();

    for (nlohmann::json& node : graph.at("nodes"))
    {
        if (!node.contains("id") || !node.contains("typeName"))
        {
            LogError("failed to load script graph from \"{}\", node requires id and typeName", path);
            return;
        }
        std::string typeName = node.at("typeName").template get<std::string>();
        uint64_t id = node.at("id").template get<uint64_t>();

        e2::Type *nodeType = e2::Type::fromName(typeName);
        if (!nodeType)
        {
            LogError("failed to load script graph from \"{}\", node has invalid typeName", path);
            return;
        }

        if (!nodeType->inherits("e2::ScriptNode"))
        {
            LogError("failed to load script graph from \"{}\", node typeName doesnt inherit ScriptNode", path);
            return;
        }

        e2::Object* nodeObject = nodeType->create();
        if (!nodeObject)
        {
            LogError("failed to load script graph from \"{}\", node could not be instanced (make sure its not virtual and is tagged dynamic)", path);
            return;
        }

        e2::ScriptNode* scriptNode = nodeObject->cast<e2::ScriptNode>();
        if (!scriptNode)
        {
            LogError("failed to load script graph from \"{}\", created node was invalid, wtf?", path);
            e2::destroy(scriptNode);
            return;
        }

        scriptNode->postConstruct(this, id);
        scriptNode->deserialize(node);
        m_nodes[id] = scriptNode;
    }

    for (nlohmann::json& connection : graph.at("connections"))
    {
        if (!connection.contains("startNode") || !connection.contains("startNodePin") || !connection.contains("endNode"))
        {
            LogError("failed to load script graph from \"{}\", connection requires startNode, startNodePin, and endNode", path);
            return;
        }
        
        uint64_t startNode = connection.at("startNode").template get<uint64_t>();
        uint64_t startNodePin = connection.at("startNodePin").template get<uint64_t>();
        uint64_t endNode = connection.at("endNode").template get<uint64_t>();

        m_connections[{startNode, startNodePin}] = endNode;
    }

    m_valid = true;

}

e2::ScriptGraph::~ScriptGraph()
{
    for (auto& [id, node] : m_nodes)
    {
        e2::destroy(node);
    }
}

e2::Game* e2::ScriptGraph::game() {
    return m_game;
}

e2::ScriptNode* e2::ScriptGraph::nodeById(ScriptNodeId id)
{
    auto finder = m_nodes.find(id);
    if (finder == m_nodes.end())
    {
        return nullptr;
    }

    return finder->second;
}

e2::ScriptNode* e2::ScriptGraph::endNodeAtPin(ScriptNodeId startNode, uint64_t startPinIndex)
{
    auto finder = m_connections.find({ startNode, startPinIndex });
    if (finder == m_connections.end())
    {
        return nullptr;
    }

    return nodeById(finder->second);
}

e2::ScriptNode* e2::ScriptGraph::entryNode()
{
    return nodeById(m_entryNode);
}

e2::ScriptExecutionContext::ScriptExecutionContext(ScriptGraph* graph)
    : m_graph(graph)
{
    m_currentNode = m_graph->entryNode();

    if (m_currentNode)
        m_currentNode->onActivate();
}

void e2::ScriptExecutionContext::update(double seconds)
{
    if (!m_currentNode)
        return;

    m_currentNode->onUpdate(seconds);

    if (m_currentNode->isDone())
    {
        m_currentNode->onDeactivate();
        m_currentNode = m_graph->endNodeAtPin(m_currentNode->id(), m_currentNode->resultPin());

        if (m_currentNode)
            m_currentNode->onActivate();
    }
}

bool e2::ScriptExecutionContext::isDone()
{
    return m_currentNode == nullptr;
}

e2::DialogueNode::~DialogueNode()
{
}

void e2::DialogueNode::serialize(nlohmann::json& obj)
{
}

void e2::DialogueNode::deserialize(nlohmann::json& obj)
{
    if (obj.contains("lines"))
    {
        for (nlohmann::json& line : obj.at("lines"))
        {
            if(!m_lines.full())
                m_lines.push({ line.template get<std::string>(), 0 });
        }
    }

    if (obj.contains("answers"))
    {
        for (nlohmann::json& answer : obj.at("answers"))
        {
            if(!m_answers.full())
                m_answers.push({ answer.template get<std::string>() });
        }
    }
}

void e2::DialogueNode::onActivate()
{
    m_selectedAnswer = 0;
    m_currentLineOffset = 0;
    for (e2::DialogueLine& line : m_lines)
    {
        line.currentCharacterOffset = 0;
    }
    m_done = false;
}

void e2::DialogueNode::onUpdate(double seconds)
{
    if (m_lines.size() == 0)
    {
        m_done = true;
        return;
    }

    e2::Game* game = m_graph->game();
    e2::GameSession* session = game->gameSession();
    e2::UIContext* ui = session->uiContext();

    bool allLinesRead = m_currentLineOffset >= m_lines.size();

    if (allLinesRead)
    {
        if (ui->keyboardState().pressed(e2::Key::W) && !m_answers.empty())
        {
            if (m_selectedAnswer > 0)
                m_selectedAnswer--;
        }
        if (ui->keyboardState().pressed(e2::Key::S) && !m_answers.empty())
        {
            if (m_selectedAnswer < m_answers.size() - 1)
                m_selectedAnswer++;
        }
    }
    if (ui->keyboardState().pressed(e2::Key::E))
    {
        if (allLinesRead)
        {
            m_done = true;
        }
        else
        {
            m_lines[m_currentLineOffset].currentCharacterOffset = m_lines[m_currentLineOffset].text.size();
            m_currentLineOffset++;
        }
    }



    
    constexpr double offsetRate = 1.0 / 20.0; // 20 characters per second
    if (m_currentLineOffset < m_lines.size())
    {
        m_secondsSinceCharacterOffset += seconds;
        while (m_secondsSinceCharacterOffset > offsetRate)
        {
            m_secondsSinceCharacterOffset -= offsetRate;
            m_lines[m_currentLineOffset].currentCharacterOffset++;

            if (m_lines[m_currentLineOffset].currentCharacterOffset > m_lines[m_currentLineOffset].text.size())
            {
                m_currentLineOffset++;

                if (m_currentLineOffset >= m_lines.size())
                {
                    break;
                }
            }
        }
    }

    allLinesRead = m_currentLineOffset >= m_lines.size();

    // calculate dialogue width 
    float maxLineWidth = 0.0f;
    for (e2::DialogueLine& line : m_lines)
    {
        float currLineWidth = ui->calculateTextWidth(e2::FontFace::Serif, 14, line.text);
        if (currLineWidth > maxLineWidth)
        {
            maxLineWidth = currLineWidth;
        }
    }

    for (e2::DialogueAnswer& answer : m_answers)
    {
        float currLineWidth = ui->calculateTextWidth(e2::FontFace::Serif, 14, answer.text);
        if (currLineWidth > maxLineWidth)
        {
            maxLineWidth = currLineWidth;
        }
    }

    constexpr float textPadding = 4.0f;
    constexpr float lineHeight = 14.0f;
    constexpr float padding = 8.0f;
    constexpr float linePadding = 4.0f;
    float dialogueWidth = maxLineWidth + padding * 2.0f + textPadding*2.0f;
    float dialogueHeight = padding
                        + (lineHeight * m_lines.size()) 
                        + (linePadding * (m_lines.size() < 2 ? 0 :  m_lines.size() - 1)) 
                        + padding
                        + (lineHeight * m_answers.size())
                        + (linePadding * (m_answers.size() < 2 ? 0 :  m_answers.size() - 1))
                        + (m_answers.empty() ? 0.0f : padding);
    glm::vec2 dialogueSize{dialogueWidth, dialogueHeight};

    glm::vec2 uiSize = ui->size();
    
    glm::vec2 uiCenter = uiSize / 2.0f;
    glm::vec2 dialogueOffset;
    dialogueOffset.x = uiCenter.x - (dialogueSize.x / 2.0f);
    dialogueOffset.y = uiSize.y - dialogueSize.y - 200.0f;


    ui->drawGamePanel(dialogueOffset, dialogueSize, false, 0.75f);

    glm::vec2 lineCursor = dialogueOffset + padding;
    int32_t i = 0;
    for (e2::DialogueLine& line : m_lines)
    {
        std::string const& renderText = line.text.substr(0, line.currentCharacterOffset);
        if (renderText.size() > 0)
        {
            ui->drawRasterText(e2::FontFace::Serif, lineHeight, e2::UIColor::white(), lineCursor + glm::vec2{ textPadding, lineHeight / 2.0f}, renderText);
        }

        lineCursor.y += lineHeight;
        
        if (i < m_lines.size() - 1)
            lineCursor.y += linePadding;

        i++;
    }
    lineCursor.y += padding;
    i = 0;
    if (allLinesRead && !m_answers.empty())
    {
        for (e2::DialogueAnswer& answer : m_answers)
        {
            std::string const& renderText = answer.text;
            if (renderText.size() > 0)
            {
                ui->drawGamePanel(lineCursor, { dialogueWidth - padding * 2.0f, 14.0f }, i == m_selectedAnswer, 0.75);
                ui->drawRasterText(e2::FontFace::Serif, 14, e2::UIColor::white(), lineCursor + glm::vec2{ textPadding, lineHeight / 2.0f }, renderText);
            }

            lineCursor.y += lineHeight;


            if (i < m_answers.size() - 1)
                lineCursor.y += linePadding;

            i++;
        }
    }



}

void e2::DialogueNode::onDeactivate()
{
}

bool e2::DialogueNode::isDone()
{
    return m_done;
}

uint64_t e2::DialogueNode::resultPin()
{
    return m_selectedAnswer;
}

uint64_t e2::DialogueNode::numOutputPins()
{
    return m_answers.size();
}

std::string const& e2::DialogueNode::outputPin(uint64_t index)
{
    return m_answers[index].text;
}
