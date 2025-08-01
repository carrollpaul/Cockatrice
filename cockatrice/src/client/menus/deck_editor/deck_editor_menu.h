#ifndef DECK_EDITOR_MENU_H
#define DECK_EDITOR_MENU_H

#include "../../tabs/abstract_tab_deck_editor.h"

#include <QMenu>

class AbstractTabDeckEditor;
class DeckEditorMenu : public QMenu
{
    Q_OBJECT
public:
    explicit DeckEditorMenu(AbstractTabDeckEditor *parent);

    AbstractTabDeckEditor *deckEditor;

    QAction *aNewDeck, *aLoadDeck, *aClearRecents, *aSaveDeck, *aSaveDeckAs, *aLoadDeckFromClipboard,
        *aEditDeckInClipboard, *aEditDeckInClipboardRaw, *aSaveDeckToClipboard, *aSaveDeckToClipboardNoSetInfo,
        *aSaveDeckToClipboardRaw, *aSaveDeckToClipboardRawNoSetInfo, *aPrintDeck, *aLoadDeckFromWebsite,
        *aExportDeckDecklist, *aExportDeckDecklistXyz, *aAnalyzeDeckDeckstats, *aAnalyzeDeckTappedout, *aClose;
    QAction *aUndo, *aRedo;
    QMenu *loadRecentDeckMenu, *analyzeDeckMenu, *editDeckInClipboardMenu, *saveDeckToClipboardMenu;

    void setSaveStatus(bool newStatus);
    void updateUndoRedoActions(bool canUndo, bool canRedo);

public slots:
    void updateRecentlyOpened();
    void actClearRecents();
    void retranslateUi();
    void refreshShortcuts();
};

#endif
