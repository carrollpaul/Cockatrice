#ifndef DECK_COMMAND_H
#define DECK_COMMAND_H

#include <QObject>
#include <QString>
#include <memory>

class DeckLoader;
class ExactCard;

/**
 * @brief Abstract base class for all deck editing commands that support undo/redo functionality.
 *
 * This class implements the Command pattern to encapsulate deck operations as objects,
 * enabling undo/redo capabilities. Each command stores the information needed to both
 * execute and reverse the operation.
 *
 * Commands are designed to be lightweight and store minimal state required for
 * execution and reversal. They operate on the deck model through the DeckLoader interface.
 */
class DeckCommand
{
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes
     */
    virtual ~DeckCommand() = default;

    /**
     * @brief Execute the command, applying the change to the deck
     *
     * This method should perform the intended operation on the deck.
     * It should be idempotent - calling execute() multiple times should
     * have the same effect as calling it once.
     *
     * @return true if the command was executed successfully, false otherwise
     */
    virtual bool execute() = 0;

    /**
     * @brief Undo the command, reversing the change made to the deck
     *
     * This method should reverse exactly what execute() did, restoring
     * the deck to its previous state. It should only be called after
     * execute() has been successfully called.
     *
     * @return true if the command was undone successfully, false otherwise
     */
    virtual bool undo() = 0;

    /**
     * @brief Get a human-readable description of this command
     *
     * This description is used for UI elements like undo/redo menu items
     * and history displays. It should be concise but descriptive enough
     * for users to understand what the command does.
     *
     * @return A QString describing the command (e.g., "Add Lightning Bolt to main deck")
     */
    virtual QString getDescription() const = 0;

    /**
     * @brief Get the type identifier for this command
     *
     * This can be used for command categorization, filtering, or special
     * handling of certain command types. Should return a stable identifier
     * that doesn't change between application runs.
     *
     * @return A QString identifying the command type (e.g., "AddCard", "RemoveCard")
     */
    virtual QString getType() const = 0;

    /**
     * @brief Check if this command can be merged with another command
     *
     * Some commands can be merged for efficiency (e.g., multiple additions
     * of the same card can be combined into a single command with a higher count).
     * This method determines if merging is possible.
     *
     * @param other The command to potentially merge with
     * @return true if the commands can be merged, false otherwise
     */
    virtual bool canMergeWith(const DeckCommand *other) const
    {
        Q_UNUSED(other)
        return false; // Default implementation: no merging
    }

    /**
     * @brief Merge this command with another compatible command
     *
     * This method should only be called if canMergeWith() returns true.
     * It should modify this command to incorporate the effect of the other command.
     *
     * @param other The command to merge with (will be deleted after merging)
     * @return true if merging was successful, false otherwise
     */
    virtual bool mergeWith(const DeckCommand *other)
    {
        Q_UNUSED(other)
        return false; // Default implementation: no merging
    }

    /**
     * @brief Check if this command actually modifies the deck
     *
     * Some operations might not result in actual changes (e.g., adding 0 cards,
     * removing a card that's not in the deck). This method allows commands to
     * indicate whether they represent a meaningful change.
     *
     * @return true if the command represents a meaningful change, false otherwise
     */
    virtual bool isModifying() const
    {
        return true; // Default implementation: assume all commands modify the deck
    }

    /**
     * @brief Get the timestamp when this command was created
     *
     * Useful for command history, debugging, and potentially for time-based
     * command expiration or grouping.
     *
     * @return The creation timestamp
     */
    qint64 getTimestamp() const
    {
        return m_timestamp;
    }

protected:
    /**
     * @brief Protected constructor to prevent direct instantiation
     *
     * Only derived classes should create DeckCommand instances.
     * Automatically sets the creation timestamp.
     */
    DeckCommand() : m_timestamp(QDateTime::currentMSecsSinceEpoch())
    {
    }

    /**
     * @brief Helper method to validate that a deck pointer is valid
     *
     * @param deck The deck pointer to validate
     * @return true if the deck is valid and can be operated on
     */
    bool isValidDeck(const DeckLoader *deck) const
    {
        return deck != nullptr;
    }

    /**
     * @brief Helper method to validate that a card is valid
     *
     * @param card The card to validate
     * @return true if the card is valid and can be operated on
     */
    bool isValidCard(const ExactCard &card) const;

private:
    qint64 m_timestamp; ///< When this command was created
};

/**
 * @brief Smart pointer type for DeckCommand objects
 *
 * Using unique_ptr ensures proper memory management and prevents
 * accidental copying of command objects.
 */
using DeckCommandPtr = std::unique_ptr<DeckCommand>;

/**
 * @brief Factory function type for creating commands
 *
 * This can be used with registration systems for command creation
 * from serialized data or configuration.
 */
using CommandFactory = std::function<DeckCommandPtr()>;

#endif