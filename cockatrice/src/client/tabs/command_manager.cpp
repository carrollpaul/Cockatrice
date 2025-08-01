#include "command_manager.h"

CommandManager::CommandManager(QObject *parent, int maxHistorySize)
    : QObject(parent), m_maxHistorySize(maxHistorySize), m_mergingEnabled(true), m_cleanupTimer(new QTimer(this))
{
    // Setup cleanup timer for non-blocking history management
    m_cleanupTimer->setSingleShot(true);
    m_cleanupTimer->setInterval(100); // 100ms delay
    connect(m_cleanupTimer, &QTimer::timeout, this, &CommandManager::cleanupHistory);
}

CommandManager::~CommandManager()
{
    clearHistory();
}

bool CommandManager::executeCommand(DeckCommandPtr command)
{
    if (!command) {
        qWarning() << "CommandManager::executeCommand: Null command provided";
        return false;
    }

    // Check if this is a modifying command
    if (!command->isModifying()) {
        qDebug() << "CommandManager::executeCommand: Skipping non-modifying command:" << command->getDescription();
        return true; // Don't execute, but don't treat as error
    }

    // Execute the command
    if (!command->execute()) {
        qWarning() << "CommandManager::executeCommand: Failed to execute command:" << command->getDescription();
        return false;
    }

    // Clear redo stack since new actions invalidate redo history
    m_redoStack.clear();

    // Try to merge with previous command if merging is enabled
    if (m_mergingEnabled && tryMergeCommand(command.get())) {
        // Command was merged, don't add to stack
        qDebug() << "CommandManager::executeCommand: Merged command:" << command->getDescription();
    } else {
        // Add command to undo stack
        m_undoStack.push(std::move(command));
        qDebug() << "CommandManager::executeCommand: Added command to stack:" << m_undoStack.top()->getDescription();
    }

    // Schedule cleanup if we're over the limit
    if (m_maxHistorySize > 0 && m_undoStack.size() > m_maxHistorySize) {
        m_cleanupTimer->start();
    }

    // Update UI state
    updateState();

    // Emit signals
    emit commandExecuted(m_undoStack.isEmpty() ? QString() : m_undoStack.top()->getDescription());

    return true;
}

bool CommandManager::undo()
{
    if (!canUndo()) {
        return false;
    }

    // Get the command to undo
    DeckCommandPtr command = std::move(m_undoStack.top());
    m_undoStack.pop();

    // Undo the command
    if (!command->undo()) {
        qWarning() << "CommandManager::undo: Failed to undo command:" << command->getDescription();
        // Put the command back on the stack
        m_undoStack.push(std::move(command));
        return false;
    }

    // Move command to redo stack
    QString description = command->getDescription();
    m_redoStack.push(std::move(command));

    // Update UI state
    updateState();

    // Emit signal
    emit commandUndone(description);

    qDebug() << "CommandManager::undo: Undone command:" << description;
    return true;
}

bool CommandManager::redo()
{
    if (!canRedo()) {
        return false;
    }

    // Get the command to redo
    DeckCommandPtr command = std::move(m_redoStack.top());
    m_redoStack.pop();

    // Re-execute the command
    if (!command->execute()) {
        qWarning() << "CommandManager::redo: Failed to redo command:" << command->getDescription();
        // Put the command back on the redo stack
        m_redoStack.push(std::move(command));
        return false;
    }

    // Move command back to undo stack
    QString description = command->getDescription();
    m_undoStack.push(std::move(command));

    // Update UI state
    updateState();

    // Emit signal
    emit commandRedone(description);

    qDebug() << "CommandManager::redo: Redone command:" << description;
    return true;
}

bool CommandManager::canUndo() const
{
    return !m_undoStack.isEmpty();
}

bool CommandManager::canRedo() const
{
    return !m_redoStack.isEmpty();
}

QString CommandManager::undoDescription() const
{
    if (canUndo()) {
        return tr("Undo %1").arg(m_undoStack.top()->getDescription());
    }
    return QString();
}

QString CommandManager::redoDescription() const
{
    if (canRedo()) {
        return tr("Redo %1").arg(m_redoStack.top()->getDescription());
    }
    return QString();
}

void CommandManager::clearHistory()
{
    m_undoStack.clear();
    m_redoStack.clear();

    updateState();
    emit historyCleared();

    qDebug() << "CommandManager::clearHistory: Command history cleared";
}

int CommandManager::undoCount() const
{
    return m_undoStack.size();
}

int CommandManager::redoCount() const
{
    return m_redoStack.size();
}

void CommandManager::setMaxHistorySize(int maxSize)
{
    m_maxHistorySize = maxSize;

    // Trigger cleanup if we're now over the limit
    if (m_maxHistorySize > 0 && m_undoStack.size() > m_maxHistorySize) {
        m_cleanupTimer->start();
    }
}

void CommandManager::cleanupHistory()
{
    if (m_maxHistorySize <= 0 || m_undoStack.size() <= m_maxHistorySize) {
        return; // No cleanup needed
    }

    int excessCommands = m_undoStack.size() - m_maxHistorySize;
    removeOldCommands(excessCommands);

    qDebug() << "CommandManager::cleanupHistory: Removed" << excessCommands << "old commands";
}

void CommandManager::updateState()
{
    bool undoAvailable = canUndo();
    bool redoAvailable = canRedo();

    emit undoRedoStateChanged(undoAvailable, redoAvailable);
    emit descriptionsChanged(undoDescription(), redoDescription());
}

bool CommandManager::tryMergeCommand(DeckCommand *command)
{
    if (m_undoStack.isEmpty()) {
        return false;
    }

    DeckCommand *topCommand = m_undoStack.top().get();
    if (!topCommand->canMergeWith(command)) {
        return false;
    }

    // Attempt to merge
    if (topCommand->mergeWith(command)) {
        qDebug() << "CommandManager::tryMergeCommand: Successfully merged commands";
        return true;
    }

    return false;
}

void CommandManager::removeOldCommands(int count)
{
    // We need to remove from the bottom of the stack, so we'll use a temporary stack
    QStack<DeckCommandPtr> tempStack;

    // Move commands we want to keep to temp stack
    int keepCount = m_undoStack.size() - count;
    for (int i = 0; i < keepCount && !m_undoStack.isEmpty(); ++i) {
        tempStack.push(std::move(m_undoStack.top()));
        m_undoStack.pop();
    }

    // Clear remaining commands (the old ones we're discarding)
    m_undoStack.clear();

    // Move kept commands back to undo stack (in reverse order to maintain stack order)
    while (!tempStack.isEmpty()) {
        m_undoStack.push(std::move(tempStack.top()));
        tempStack.pop();
    }
}