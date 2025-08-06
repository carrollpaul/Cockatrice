#include "deck_command.h"

#include "../../game/cards/card_info.h"
#include "../../game/cards/exact_card.h"

bool DeckCommand::isValidCard(const ExactCard &card) const
{
    if (!card) {
        return false;
    }

    CardInfoPtr cardInfo = card.getCardPtr();
    if (!cardInfo) {
        return false;
    }

    if (cardInfo->getName().isEmpty()) {
        return false;
    }

    return true;
}