#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include "deck_command.h"

#include <QDebug>
#include <QObject>
#include <QStack>
#include <QTimer>

/**
 * @brief Manages the execution and undo/redo functionality for deck commands
 *
 * The CommandManager maintains two stacks for undo and redo operations,
 * handles command execution, merging, and provides signals for UI updates.
 * It also manages memory cleanup and optional history limits.
 */
class CommandManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Construct a CommandManager
     * @param parent The parent QObject
     * @param maxHistorySize Maximum number of commands to keep in history (0 = unlimited)
     */
    explicit CommandManager(QObject *parent = nullptr, int maxHistorySize = 100);

    /**
     * @brief Destructor - cleans up command stacks
     */
    ~CommandManager();

    /**
     * @brief Execute a command and add it to the undo stack
     *
     * This method will:
     * 1. Execute the command
     * 2. Clear the redo stack (since new actions invalidate redo history)
     * 3. Try to merge with the previous command if possible
     * 4. Add to undo stack if not merged
     * 5. Enforce history size limits
     * 6. Emit appropriate signals
     *
     * @param command The command to execute (takes ownership)
     * @return true if the command was executed successfully
     */
    bool executeCommand(DeckCommandPtr command);

    /**
     * @brief Undo the last executed command
     * @return true if an undo operation was performed
     */
    bool undo();

    /**
     * @brief Redo the last undone command
     * @return true if a redo operation was performed
     */
    bool redo();

    /**
     * @brief Check if undo is available
     * @return true if there are commands that can be undone
     */
    bool canUndo() const;

    /**
     * @brief Check if redo is available
     * @return true if there are commands that can be redone
     */
    bool canRedo() const;

    /**
     * @brief Get description of the next command that would be undone
     * @return Description string, or empty if no undo available
     */
    QString undoDescription() const;

    /**
     * @brief Get description of the next command that would be redone
     * @return Description string, or empty if no redo available
     */
    QString redoDescription() const;

    /**
     * @brief Clear all command history
     *
     * This will clear both undo and redo stacks and emit appropriate signals.
     */
    void clearHistory();

    /**
     * @brief Get the current number of commands in undo history
     * @return Number of commands that can be undone
     */
    int undoCount() const;

    /**
     * @brief Get the current number of commands in redo history
     * @return Number of commands that can be redone
     */
    int redoCount() const;

    /**
     * @brief Set the maximum history size
     * @param maxSize Maximum number of commands to keep (0 = unlimited)
     */
    void setMaxHistorySize(int maxSize);

    /**
     * @brief Get the maximum history size
     * @return Current maximum history size (0 = unlimited)
     */
    int maxHistorySize() const
    {
        return m_maxHistorySize;
    }

    /**
     * @brief Enable or disable command merging
     * @param enabled Whether to attempt command merging
     */
    void setMergingEnabled(bool enabled)
    {
        m_mergingEnabled = enabled;
    }

    /**
     * @brief Check if command merging is enabled
     * @return true if command merging is enabled
     */
    bool isMergingEnabled() const
    {
        return m_mergingEnabled;
    }

signals:
    /**
     * @brief Emitted when the undo/redo state changes
     * @param canUndo Whether undo is available
     * @param canRedo Whether redo is available
     */
    void undoRedoStateChanged(bool canUndo, bool canRedo);

    /**
     * @brief Emitted when undo/redo descriptions change
     * @param undoDescription Description of next undo command
     * @param redoDescription Description of next redo command
     */
    void descriptionsChanged(const QString &undoDescription, const QString &redoDescription);

    /**
     * @brief Emitted when a command is successfully executed
     * @param description Description of the executed command
     */
    void commandExecuted(const QString &description);

    /**
     * @brief Emitted when a command is successfully undone
     * @param description Description of the undone command
     */
    void commandUndone(const QString &description);

    /**
     * @brief Emitted when a command is successfully redone
     * @param description Description of the redone command
     */
    void commandRedone(const QString &description);

    /**
     * @brief Emitted when the command history is cleared
     */
    void historyCleared();

private slots:
    /**
     * @brief Cleanup old commands when history size limit is exceeded
     */
    void cleanupHistory();

private:
    /**
     * @brief Update the UI state signals
     */
    void updateState();

    /**
     * @brief Try to merge a command with the top of the undo stack
     * @param command The command to potentially merge
     * @return true if the command was merged (and should not be added to stack)
     */
    bool tryMergeCommand(DeckCommand *command);

    /**
     * @brief Remove and delete commands from the bottom of the undo stack
     * @param count Number of commands to remove
     */
    void removeOldCommands(int count);

    QStack<DeckCommandPtr> m_undoStack; ///< Stack of commands that can be undone
    QStack<DeckCommandPtr> m_redoStack; ///< Stack of commands that can be redone

    int m_maxHistorySize;  ///< Maximum number of commands to keep (0 = unlimited)
    bool m_mergingEnabled; ///< Whether to attempt command merging

    QTimer *m_cleanupTimer; ///< Timer for delayed cleanup operations
};

#endif