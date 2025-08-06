#ifndef TAB_GENERIC_DECK_EDITOR_H
#define TAB_GENERIC_DECK_EDITOR_H

#include "../../game/cards/card_info.h"
#include "../menus/deck_editor/deck_editor_menu.h"
#include "../ui/widgets/deck_editor/deck_editor_card_info_dock_widget.h"
#include "../ui/widgets/deck_editor/deck_editor_database_display_widget.h"
#include "../ui/widgets/deck_editor/deck_editor_deck_dock_widget.h"
#include "../ui/widgets/deck_editor/deck_editor_filter_dock_widget.h"
#include "../ui/widgets/deck_editor/deck_editor_printing_selector_dock_widget.h"
#include "../ui/widgets/visual_deck_storage/deck_preview/deck_preview_deck_tags_display_widget.h"
#include "command_manager.h"
#include "tab.h"

class CardDatabaseModel;
class CardDatabaseDisplayModel;
class CardInfoFrameWidget;
class DeckLoader;
class DeckEditorMenu;
class DeckEditorCardInfoDockWidget;
class DeckEditorDatabaseDisplayWidget;
class DeckEditorDeckDockWidget;
class DeckEditorFilterDockWidget;
class DeckEditorPrintingSelectorDockWidget;
class DeckPreviewDeckTagsDisplayWidget;
class Response;
class FilterTreeModel;
class FilterBuilder;
class CommandManager;

class QTreeView;
class QTextEdit;
class QLabel;
class QComboBox;
class QGroupBox;
class QMessageBox;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;
class QDockWidget;
class QMenu;
class QAction;

/**
 * @brief Abstract base class for deck editor tabs in Cockatrice
 *
 * This class provides the core functionality for deck editing interfaces, implementing
 * the Command pattern for undo/redo operations and managing multiple dock widgets for
 * different aspects of deck building (card database, deck list, card info, filters).
 *
 * The class follows Qt's Model-View architecture and provides a flexible framework
 * for different deck editor implementations while maintaining consistent behavior
 * across various deck formats and game types.
 */
class AbstractTabDeckEditor : public Tab
{
    Q_OBJECT

    friend class DeckEditorMenu;

public:
    /**
     * @brief Constructs a new deck editor tab
     * @param _tabSupervisor Parent tab supervisor that manages this tab
     */
    explicit AbstractTabDeckEditor(TabSupervisor *_tabSupervisor);

    // UI and Navigation

    /**
     * @brief Creates the menu system for this deck editor (pure virtual)
     */
    virtual void createMenus() = 0;

    /**
     * @brief Gets the display text for this tab
     * @return Tab title text, typically includes deck name and modification status
     */
    [[nodiscard]] virtual QString getTabText() const override = 0;

    /**
     * @brief Shows save confirmation dialog if deck has unsaved changes
     * @return true if user wants to proceed (saved or discarded), false if cancelled
     */
    bool confirmClose();

    /**
     * @brief Updates all translatable UI text (pure virtual)
     *
     * Called when application language changes to update all user-visible strings.
     */
    virtual void retranslateUi() override = 0;

    // Deck Management

    /**
     * @brief Opens a deck in this editor tab
     * @param deck The deck to open (takes ownership of the object)
     *
     * Loads the deck into the editor, updates all UI components, and marks
     * the deck as unmodified. Updates recent files list if deck has a filename.
     */
    void openDeck(DeckLoader *deck);

    /**
     * @brief Gets the currently loaded deck
     * @return Pointer to the current DeckLoader, or nullptr if no deck loaded
     */
    DeckLoader *getDeckList() const;

    /**
     * @brief Sets the modification status of the current deck
     * @param _windowModified true if deck has unsaved changes, false otherwise
     *
     * Updates the tab title to show modification status (typically with asterisk)
     * and enables/disables save-related UI elements appropriately.
     */
    void setModified(bool _windowModified);

    // UI Elements - Public dock widgets for external access

    DeckEditorMenu *deckMenu;                                         ///< Main menu for deck operations
    DeckEditorDatabaseDisplayWidget *databaseDisplayDockWidget;       ///< Card database browser and search
    DeckEditorCardInfoDockWidget *cardInfoDockWidget;                 ///< Detailed card information display
    DeckEditorDeckDockWidget *deckDockWidget;                         ///< Deck list and composition view
    DeckEditorFilterDockWidget *filterDockWidget;                     ///< Card filtering and search options
    DeckEditorPrintingSelectorDockWidget *printingSelectorDockWidget; ///< Card printing/edition selector

public slots:
    /**
     * @brief Called when deck structure changes (virtual)
     *
     * Override in derived classes to handle deck structure changes.
     * Base implementation is empty.
     */
    virtual void onDeckChanged();

    /**
     * @brief Called when deck content is modified
     *
     * Updates modification status and save button state based on whether
     * the deck is blank/new or contains actual content.
     */
    virtual void onDeckModified();

    /**
     * @brief Updates the displayed card information
     * @param card The card to display information for
     *
     * Updates both the card info dock and printing selector with the new card.
     */
    void updateCard(const ExactCard &card);

    /**
     * @brief Updates undo/redo button states
     * @param canUndo true if undo operation is available
     * @param canRedo true if redo operation is available
     *
     * Called by CommandManager when command history state changes.
     */
    void updateUndoRedoState(bool canUndo, bool canRedo);

    /**
     * @brief Adds cards to the deck
     * @param card The card to add
     * @param zone The deck zone to add to (defaults to main deck)
     * @param count The number of copies to add (defaults to 1)
     *
     * Token cards are automatically redirected to tokens zone.
     */
    void actAddCard(const ExactCard &card, const QString &zone = DECK_ZONE_MAIN, int count = 1);

    /**
     * @brief Removes cards from the deck
     * @param card The card to remove
     * @param zone The deck zone to remove from (defaults to main deck)
     * @param count The number of copies to remove (defaults to 1)
     *
     * Token cards are automatically handled in tokens zone regardless of specified zone.
     */
    void actRemoveCard(const ExactCard &card, const QString &zone = DECK_ZONE_MAIN, int count = 1);

    /**
     * @brief Removes all copies of a card from the specified zone
     * @param card The card to remove all copies of
     * @param zone The deck zone to remove from (defaults to main deck)
     *
     * Token cards are automatically handled in tokens zone regardless of specified zone.
     */
    void actRemoveAllCard(const ExactCard &card, const QString &zone = DECK_ZONE_MAIN);

    /**
     * @brief Swaps all instances of a card between two zones
     * @param card The card to swap
     * @param currentZone The zone in which the currently resides
     *
     * Moves ALL instances of the card from fromZone to toZone.
     * Token cards are automatically handled in tokens zone.
     */
    void actSwapCard(const ExactCard &card, const QString &currentZone);

    /**
     * @brief Opens a recently used deck file
     * @param fileName Path to the deck file to open
     *
     * Checks for unsaved changes before opening and handles file format detection.
     */
    void actOpenRecent(const QString &fileName);

    /**
     * @brief Updates the card database filter
     * @param filterTree New filter configuration to apply
     *
     * Propagates filter changes to the database display widget.
     */
    void filterTreeChanged(FilterTree *filterTree);

    /**
     * @brief Handles tab close request
     * @return true if tab can be closed, false if user cancelled
     *
     * Checks for unsaved changes and prompts user if necessary.
     */
    bool closeRequest() override;

    /**
     * @brief Shows the printing selector for current card (pure virtual)
     *
     * Derived classes implement this to show printing selection UI.
     */
    virtual void showPrintingSelector() = 0;

    /**
     * @brief Handles dock widget top-level state changes (pure virtual)
     * @param topLevel true if dock became floating, false if docked
     *
     * Derived classes can implement special handling for floating docks.
     */
    virtual void dockTopLevelChanged(bool topLevel) = 0;

signals:
    /**
     * @brief Emitted when requesting to open a deck in a new editor tab
     * @param deckLoader The deck to open, or nullptr for new blank deck
     */
    void openDeckEditor(const DeckLoader *deckLoader);

    /**
     * @brief Emitted when this deck editor tab is being closed
     * @param tab Pointer to this tab being closed
     */
    void deckEditorClosing(AbstractTabDeckEditor *tab);

protected slots:
    // Deck Operations

    /**
     * @brief Creates a new blank deck
     *
     * Prompts to save current deck if modified, then creates a new empty deck.
     * May open in new tab depending on user preferences.
     */
    virtual void actNewDeck();

    /**
     * @brief Clears current deck and resets modification status
     *
     * Internal helper for creating new decks without save prompts.
     */
    void cleanDeckAndResetModified();

    /**
     * @brief Shows file dialog to load a deck from file (virtual)
     *
     * Handles save confirmation, file selection, and deck loading.
     */
    virtual void actLoadDeck();

    /**
     * @brief Saves the current deck to its current location
     * @return true if save successful, false if failed or cancelled
     *
     * Uses last known filename/format, or prompts for location if new deck.
     * Handles both local files and remote deck storage.
     */
    bool actSaveDeck();

    /**
     * @brief Shows file dialog to save deck to a new location (virtual)
     * @return true if save successful, false if failed or cancelled
     */
    virtual bool actSaveDeckAs();

    /**
     * @brief Loads a deck from clipboard text (virtual)
     *
     * Parses various deck formats from clipboard and loads into editor.
     */
    virtual void actLoadDeckFromClipboard();

    /**
     * @brief Opens deck in clipboard for editing with annotations
     *
     * Allows editing deck text with set names and comments before importing.
     */
    void actEditDeckInClipboard();

    /**
     * @brief Opens deck in clipboard for raw text editing
     *
     * Allows editing plain deck text without annotations before importing.
     */
    void actEditDeckInClipboardRaw();

    /**
     * @brief Copies current deck to clipboard with annotations
     *
     * Includes set names, comments, and formatting in clipboard text.
     */
    void actSaveDeckToClipboard();

    /**
     * @brief Copies current deck to clipboard without set information
     *
     * Plain card names and quantities only, no set/edition info.
     */
    void actSaveDeckToClipboardNoSetInfo();

    /**
     * @brief Copies current deck to clipboard as raw text with set info
     *
     * Minimal formatting with set information included.
     */
    void actSaveDeckToClipboardRaw();

    /**
     * @brief Copies current deck to clipboard as raw text without set info
     *
     * Minimal formatting, card names and quantities only.
     */
    void actSaveDeckToClipboardRawNoSetInfo();

    /**
     * @brief Shows print preview dialog for current deck
     *
     * Allows printing formatted deck lists with various layout options.
     */
    void actPrintDeck();

    /**
     * @brief Shows dialog to import deck from website URL (virtual)
     *
     * Supports various deck sharing websites and URL formats.
     */
    virtual void actLoadDeckFromWebsite();

    /**
     * @brief Exports current deck to decklist.org website
     *
     * Opens browser with deck data formatted for the legacy decklist site.
     */
    void actExportDeckDecklist();

    /**
     * @brief Exports current deck to decklist.xyz website
     *
     * Opens browser with deck data formatted for the new decklist site.
     */
    void actExportDeckDecklistXyz();

    /**
     * @brief Analyzes current deck using DeckStats service
     *
     * Opens external analysis tool with current deck data.
     */
    void actAnalyzeDeckDeckstats();

    /**
     * @brief Analyzes current deck using TappedOut service
     *
     * Opens external analysis tool with current deck data.
     */
    void actAnalyzeDeckTappedout();

    // Remote Save

    /**
     * @brief Handles completion of remote deck save operation
     * @param r Server response from save operation
     *
     * Updates UI state based on save success/failure.
     */
    void saveDeckRemoteFinished(const Response &r);

    // UI Layout Management

    /**
     * @brief Loads the saved UI layout (pure virtual)
     *
     * Restores dock positions, sizes, and visibility from settings.
     */
    virtual void loadLayout() = 0;

    /**
     * @brief Resets UI layout to default configuration (pure virtual)
     *
     * Restores all docks to their default positions and sizes.
     */
    virtual void restartLayout() = 0;

    /**
     * @brief Allows dock widgets to resize freely (pure virtual)
     *
     * Removes size constraints to allow user resizing of dock widgets.
     */
    virtual void freeDocksSize() = 0;

    /**
     * @brief Updates all keyboard shortcuts (pure virtual)
     *
     * Called when shortcut settings change to update all action shortcuts.
     */
    virtual void refreshShortcuts() = 0;

    /**
     * @brief Handles tab close event
     * @param event Close event (accepted by default)
     *
     * Emits closing signal and accepts the close event.
     */
    void closeEvent(QCloseEvent *event) override;

    /**
     * @brief Filters events for dock widgets
     * @param o Object that received the event
     * @param e The event to filter
     * @return true if event was handled, false to continue processing
     *
     * Synchronizes dock visibility menu items with actual dock state.
     */
    bool eventFilter(QObject *o, QEvent *e) override;

    /**
     * @brief Handles dock visibility menu actions (pure virtual)
     *
     * Shows/hides dock widgets based on menu selection.
     */
    virtual void dockVisibleTriggered() = 0;

    /**
     * @brief Handles dock floating menu actions (pure virtual)
     *
     * Toggles dock widgets between floating and docked states.
     */
    virtual void dockFloatingTriggered() = 0;

private:
    /**
     * @brief Sets the current deck (internal method)
     * @param _deck The deck to set (takes ownership)
     *
     * Updates all UI components with new deck data and caches card images.
     */
    virtual void setDeck(DeckLoader *_deck);

    /**
     * @brief Helper for clipboard editing operations
     * @param annotated true to include annotations, false for raw text
     */
    void editDeckInClipboard(bool annotated);

    /**
     * @brief Helper for deck export to various websites
     * @param website Target website for export
     */
    void exportToDecklistWebsite(DeckLoader::DecklistWebsite website);

protected:
    /**
     * @brief Enum for controlling where to open decks
     */
    enum DeckOpenLocation
    {
        CANCELLED, ///< User cancelled the operation
        SAME_TAB,  ///< Open in current tab
        NEW_TAB    ///< Open in new tab
    };

    /**
     * @brief Shows save confirmation dialog before opening new deck
     * @param openInSameTabIfBlank Open in same tab if current deck is blank
     * @return Where to open the deck or CANCELLED if user cancelled
     */
    DeckOpenLocation confirmOpen(bool openInSameTabIfBlank = true);

    /**
     * @brief Creates the standard save confirmation dialog
     * @return Configured message box for save confirmation
     */
    QMessageBox *createSaveConfirmationWindow();

    /**
     * @brief Checks if current deck is a blank new deck
     * @return true if deck is unmodified and newly created
     */
    bool isBlankNewDeck() const;

    /**
     * @brief Opens a deck file with specified location preference (virtual)
     * @param fileName Path to deck file
     * @param deckOpenLocation Where to open the deck
     */
    virtual void openDeckFromFile(const QString &fileName, DeckOpenLocation deckOpenLocation);

    /**
     * @brief Gets the command manager for undo/redo operations
     * @return Pointer to the command manager instance
     */
    CommandManager *getCommandManager() const
    {
        return m_commandManager;
    }

    // UI Menu Elements

    /// View menu and dock-related submenus
    QMenu *viewMenu, *cardInfoDockMenu, *deckDockMenu, *filterDockMenu, *printingSelectorDockMenu;

    /// Layout reset action
    QAction *aResetLayout;

    /// Dock visibility and floating state actions
    QAction *aCardInfoDockVisible, *aCardInfoDockFloating, *aDeckDockVisible, *aDeckDockFloating;
    QAction *aFilterDockVisible, *aFilterDockFloating, *aPrintingSelectorDockVisible, *aPrintingSelectorDockFloating;

    /// Current modification state
    bool modified = false;

private:
    /// Command manager for undo/redo functionality
    CommandManager *m_commandManager;
};

#endif