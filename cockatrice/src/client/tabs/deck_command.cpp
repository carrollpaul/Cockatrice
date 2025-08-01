#include "deck_command.h"

#include "../../deck/deck_loader.h"
#include "../../game/cards/card_info.h"
#include "../../game/cards/exact_card.h"

#include <QDateTime>

bool DeckCommand::isValidCard(const ExactCard &card) const
{
    if (!card) {
        return false;
    }

    const CardInfo *cardInfo = card.getCardPtr();
    if (!cardInfo) {
        return false;
    }

    if (cardInfo->getName().isEmpty()) {
        return false;
    }

    return true;
}