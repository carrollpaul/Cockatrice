#ifndef CONCRETE_DECK_COMMANDS_H
#define CONCRETE_DECK_COMMANDS_H

#include "../../deck/deck_list_model.h"
#include "../../deck/deck_loader.h"
#include "../../game/cards/exact_card.h"
#include "deck_command.h"

#include <QModelIndex>

/**
 * @brief Command to add a card to a specific zone in the deck
 */
class AddCardCommand : public DeckCommand
{
public:
    /**
     * @brief Construct an AddCardCommand
     * @param deck The deck to modify
     * @param card The card to add
     * @param zone The zone to add to (e.g., DECK_ZONE_MAIN, DECK_ZONE_SIDE)
     * @param count Number of copies to add (default: 1)
     */
    AddCardCommand(DeckListModel *model, const ExactCard &card, const QString &zone, int count = 1)
        : m_deck(model), m_card(card), m_zone(zone), m_count(count), m_executed(false)
    {
    }

    bool execute() override
    {
        if (!m_deck || !isValidCard(m_card) || m_count <= 0) {
            return false;
        }

        // Add the specified number of cards
        for (int i = 0; i < m_count; ++i) {
            QModelIndex index = m_deck->addCard(m_card, m_zone);
            m_addedIndexes.append(index);
        }

        m_executed = true;
        return true;
    }

    bool undo() override
    {
        if (!m_executed || !m_deck || !isValidCard(m_card)) {
            return false;
        }

        // Remove the cards we added by decrementing their count
        for (const QModelIndex &index : m_addedIndexes) {
            if (index.isValid()) {
                const QModelIndex numberIndex = index.sibling(index.row(), 0);
                int currentCount = m_deck->data(numberIndex, Qt::EditRole).toInt();
                if (currentCount > 1) {
                    m_deck->setData(numberIndex, currentCount - 1, Qt::EditRole);
                } else {
                    m_deck->removeRow(index.row(), index.parent());
                }
            }
        }

        m_addedIndexes.clear();
        m_executed = false;
        return true;
    }

    QString getDescription() const override
    {
        if (m_count == 1) {
            return QObject::tr("Add %1 to %2").arg(m_card.getName(), getZoneDisplayName(m_zone));
        } else {
            return QObject::tr("Add %1x %2 to %3").arg(m_count).arg(m_card.getName(), getZoneDisplayName(m_zone));
        }
    }

    QString getType() const override
    {
        return "AddCard";
    }

    bool canMergeWith(const DeckCommand *other) const override
    {
        const AddCardCommand *otherAdd = dynamic_cast<const AddCardCommand *>(other);
        if (!otherAdd) {
            return false;
        }

        // Can merge if same card, same zone, same model, and both are recent
        return m_deck == otherAdd->m_deck && m_card.getName() == otherAdd->m_card.getName() &&
               m_zone == otherAdd->m_zone && qAbs(getTimestamp() - otherAdd->getTimestamp()) < 5000; // Within 5 seconds
    }

    bool mergeWith(const DeckCommand *other) override
    {
        const AddCardCommand *otherAdd = dynamic_cast<const AddCardCommand *>(other);
        if (!canMergeWith(other) || !otherAdd) {
            return false;
        }

        m_count += otherAdd->m_count;
        return true;
    }

    bool isModifying() const override
    {
        return m_count > 0 && isValidCard(m_card);
    }

private:
    DeckListModel *m_deck;
    ExactCard m_card;
    QString m_zone;
    int m_count;
    bool m_executed;
    QList<QModelIndex> m_addedIndexes; // Track the added card indexes for proper undo

    QString getZoneDisplayName(const QString &zone) const
    {
        if (zone == "main")
            return QObject::tr("main deck");
        if (zone == "side")
            return QObject::tr("sideboard");
        if (zone == "tokens")
            return QObject::tr("tokens");
        return zone;
    }
};

/**
 * @brief Command to remove/decrement a card from a specific zone in the deck
 */
class RemoveCardCommand : public DeckCommand
{
public:
    /**
     * @brief Construct a RemoveCardCommand
     * @param deck The deck to modify
     * @param card The card to remove
     * @param zone The zone to remove from
     * @param count Number of copies to remove (default: 1)
     */
    RemoveCardCommand(DeckListModel *model, const ExactCard &card, const QString &zone, int count = 1)
        : m_deck(model), m_card(card), m_zone(zone), m_count(count), m_executed(false)
    {
    }

    bool execute() override
    {
        if (!m_deck || !isValidCard(m_card) || m_count <= 0) {
            return false;
        }

        m_actuallyRemoved = 0;
        m_removedCards.clear();

        // Find and remove the specified number of cards
        for (int i = 0; i < m_count; ++i) {
            QString providerId = m_card.getPrinting().getUuid();
            QString collectorNumber = m_card.getPrinting().getProperty("num");

            QModelIndex index = m_deck->findCard(m_card.getName(), m_zone, providerId, collectorNumber);
            if (!index.isValid()) {
                index = m_deck->findCard(m_card.getName(), m_zone); // Fallback to any printing
            }

            if (index.isValid()) {
                const QModelIndex numberIndex = index.sibling(index.row(), 0);
                int currentCount = m_deck->data(numberIndex, Qt::EditRole).toInt();

                if (currentCount > 1) {
                    // Decrement count
                    m_deck->setData(numberIndex, currentCount - 1, Qt::EditRole);
                    m_removedCards.append({index, false}); // false = decremented, not removed
                } else {
                    // Remove the entire row
                    m_deck->removeRow(index.row(), index.parent());
                    m_removedCards.append({index, true}); // true = row removed
                }
                m_actuallyRemoved++;
            } else {
                // No more cards to remove
                break;
            }
        }

        m_executed = true;
        return m_actuallyRemoved > 0;
    }

    bool undo() override
    {
        if (!m_executed || !m_deck || !isValidCard(m_card)) {
            return false;
        }

        // Restore the cards we removed
        // Note: We need to restore in reverse order to maintain proper indexing
        for (int i = m_removedCards.size() - 1; i >= 0; --i) {
            const auto &removedCard = m_removedCards[i];
            if (removedCard.second) {
                // Row was removed - add the card back
                m_deck->addCard(m_card, m_zone);
            } else {
                // Count was decremented - increment it back
                QModelIndex index = m_deck->findCard(m_card.getName(), m_zone);
                if (index.isValid()) {
                    const QModelIndex numberIndex = index.sibling(index.row(), 0);
                    int currentCount = m_deck->data(numberIndex, Qt::EditRole).toInt();
                    m_deck->setData(numberIndex, currentCount + 1, Qt::EditRole);
                }
            }
        }

        m_removedCards.clear();
        m_executed = false;
        return true;
    }

    QString getDescription() const override
    {
        if (m_count == 1) {
            return QObject::tr("Remove %1 from %2").arg(m_card.getName(), getZoneDisplayName(m_zone));
        } else {
            return QObject::tr("Remove %1x %2 from %3").arg(m_count).arg(m_card.getName(), getZoneDisplayName(m_zone));
        }
    }

    QString getType() const override
    {
        return "RemoveCard";
    }

    bool canMergeWith(const DeckCommand *other) const override
    {
        const RemoveCardCommand *otherRemove = dynamic_cast<const RemoveCardCommand *>(other);
        if (!otherRemove) {
            return false;
        }

        // Can merge if same card, same zone, same model, and both are recent
        return m_deck == otherRemove->m_deck && m_card.getName() == otherRemove->m_card.getName() &&
               m_zone == otherRemove->m_zone &&
               qAbs(getTimestamp() - otherRemove->getTimestamp()) < 5000; // Within 5 seconds
    }

    bool mergeWith(const DeckCommand *other) override
    {
        const RemoveCardCommand *otherRemove = dynamic_cast<const RemoveCardCommand *>(other);
        if (!canMergeWith(other) || !otherRemove) {
            return false;
        }

        m_count += otherRemove->m_count;
        return true;
    }

    bool isModifying() const override
    {
        return m_count > 0 && isValidCard(m_card);
    }

private:
    DeckListModel *m_deck;
    ExactCard m_card;
    QString m_zone;
    int m_count;
    int m_actuallyRemoved = 0;
    bool m_executed;
    QList<QPair<QModelIndex, bool>> m_removedCards; // Track what was removed: (index, wasRowRemoved)

    QString getZoneDisplayName(const QString &zone) const
    {
        if (zone == "main")
            return QObject::tr("main deck");
        if (zone == "side")
            return QObject::tr("sideboard");
        if (zone == "tokens")
            return QObject::tr("tokens");
        return zone;
    }
};

/**
 * @brief Command to swap/move a card between zones (e.g., main deck to sideboard)
 */
class SwapCardCommand : public DeckCommand
{
public:
    /**
     * @brief Construct a SwapCardCommand
     * @param deck The deck to modify
     * @param card The card to move
     * @param fromZone The source zone
     * @param toZone The destination zone
     * @param count Number of copies to move (default: 1)
     */
    SwapCardCommand(DeckListModel *model,
                    const ExactCard &card,
                    const QString &fromZone,
                    const QString &toZone,
                    int count = 1)
        : m_deck(model), m_card(card), m_fromZone(fromZone), m_toZone(toZone), m_count(count), m_executed(false)
    {
    }

    bool execute() override
    {
        if (!m_deck || !isValidCard(m_card) || m_count <= 0) {
            return false;
        }

        if (m_fromZone == m_toZone) {
            return false; // No-op if zones are the same
        }

        m_actuallyMoved = 0;
        m_movedCards.clear();

        // Move the specified number of cards from source to destination
        for (int i = 0; i < m_count; ++i) {
            QString providerId = m_card.getPrinting().getUuid();
            QString collectorNumber = m_card.getPrinting().getProperty("num");

            QModelIndex fromIndex = m_deck->findCard(m_card.getName(), m_fromZone, providerId, collectorNumber);
            if (!fromIndex.isValid()) {
                fromIndex = m_deck->findCard(m_card.getName(), m_fromZone);
            }

            if (fromIndex.isValid()) {
                const QModelIndex numberIndex = fromIndex.sibling(fromIndex.row(), 0);
                int currentCount = m_deck->data(numberIndex, Qt::EditRole).toInt();

                // Remove from source
                bool wasRowRemoved = false;
                if (currentCount > 1) {
                    m_deck->setData(numberIndex, currentCount - 1, Qt::EditRole);
                } else {
                    m_deck->removeRow(fromIndex.row(), fromIndex.parent());
                    wasRowRemoved = true;
                }

                // Add to destination
                QModelIndex toIndex = m_deck->addCard(m_card, m_toZone);
                m_movedCards.append({fromIndex, toIndex, wasRowRemoved});
                m_actuallyMoved++;
            } else {
                break;
            }
        }

        m_executed = true;
        return m_actuallyMoved > 0;
    }

    bool undo() override
    {
        if (!m_executed || !m_deck || !isValidCard(m_card)) {
            return false;
        }

        // Restore the cards we moved (in reverse order)
        for (int i = m_movedCards.size() - 1; i >= 0; --i) {
            const auto &moveInfo = m_movedCards[i];

            // Remove from destination zone
            QModelIndex toIndex = m_deck->findCard(m_card.getName(), m_toZone);
            if (toIndex.isValid()) {
                const QModelIndex numberIndex = toIndex.sibling(toIndex.row(), 0);
                int currentCount = m_deck->data(numberIndex, Qt::EditRole).toInt();
                if (currentCount > 1) {
                    m_deck->setData(numberIndex, currentCount - 1, Qt::EditRole);
                } else {
                    m_deck->removeRow(toIndex.row(), toIndex.parent());
                }
            }

            // Restore to source zone
            if (moveInfo.wasRowRemoved) {
                m_deck->addCard(m_card, m_fromZone);
            } else {
                QModelIndex fromIndex = m_deck->findCard(m_card.getName(), m_fromZone);
                if (fromIndex.isValid()) {
                    const QModelIndex numberIndex = fromIndex.sibling(fromIndex.row(), 0);
                    int currentCount = m_deck->data(numberIndex, Qt::EditRole).toInt();
                    m_deck->setData(numberIndex, currentCount + 1, Qt::EditRole);
                }
            }
        }

        m_movedCards.clear();
        m_executed = false;
        return true;
    }

    QString getDescription() const override
    {
        if (m_count == 1) {
            return QObject::tr("Move %1 from %2 to %3")
                .arg(m_card.getName(), getZoneDisplayName(m_fromZone), getZoneDisplayName(m_toZone));
        } else {
            return QObject::tr("Move %1x %2 from %3 to %4")
                .arg(m_count)
                .arg(m_card.getName(), getZoneDisplayName(m_fromZone), getZoneDisplayName(m_toZone));
        }
    }

    QString getType() const override
    {
        return "SwapCard";
    }

    bool canMergeWith(const DeckCommand *other) const override
    {
        const SwapCardCommand *otherSwap = dynamic_cast<const SwapCardCommand *>(other);
        if (!otherSwap) {
            return false;
        }

        // Can merge if same card, same zones, same model, and both are recent
        return m_deck == otherSwap->m_deck && m_card.getName() == otherSwap->m_card.getName() &&
               m_fromZone == otherSwap->m_fromZone && m_toZone == otherSwap->m_toZone &&
               qAbs(getTimestamp() - otherSwap->getTimestamp()) < 5000; // Within 5 seconds
    }

    bool mergeWith(const DeckCommand *other) override
    {
        const SwapCardCommand *otherSwap = dynamic_cast<const SwapCardCommand *>(other);
        if (!canMergeWith(other) || !otherSwap) {
            return false;
        }

        m_count += otherSwap->m_count;
        return true;
    }

    bool isModifying() const override
    {
        return m_count > 0 && isValidCard(m_card) && m_fromZone != m_toZone;
    }

private:
    struct MoveInfo
    {
        QModelIndex fromIndex;
        QModelIndex toIndex;
        bool wasRowRemoved;
    };

    DeckListModel *m_deck;
    ExactCard m_card;
    QString m_fromZone;
    QString m_toZone;
    int m_count;
    int m_actuallyMoved = 0;
    bool m_executed;
    QList<MoveInfo> m_movedCards; // Track the moved cards for proper undo

    QString getZoneDisplayName(const QString &zone) const
    {
        if (zone == "main")
            return QObject::tr("main deck");
        if (zone == "side")
            return QObject::tr("sideboard");
        if (zone == "tokens")
            return QObject::tr("tokens");
        return zone;
    }
};

#endif