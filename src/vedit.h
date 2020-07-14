#ifndef VEDIT_H
#define VEDIT_H

#include <QTextEdit>
#include <QString>
#include <QPointer>
#include <QVector>
#include <QList>
#include <QColor>
#include <QRect>
#include <QFontMetrics>
#include "vconstants.h"
#include "vnotefile.h"
#include "veditconfig.h"
#include "veditor.h"

class VEditOperations;
class QLabel;
class QTimer;
class VVim;
class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;

class LineNumberArea;

class VEdit : public QTextEdit
{
    Q_OBJECT
public:
    VEdit(VFile *p_file, QWidget *p_parent = 0);
    virtual ~VEdit();
    virtual void beginEdit();
    virtual void endEdit();
    // Save buffer content to VFile.
    virtual void saveFile();
    virtual void setModified(bool p_modified);
    bool isModified() const;
    virtual void reloadFile();

    virtual bool scrollToBlock(int p_blockNumber);

    // User requests to insert an image.
    virtual void insertImage();

    // User requests to insert a link.
    virtual void insertLink();

    // Used for incremental search.
    // User has enter the content to search, but does not enter the "find" button yet.
    bool peekText(const QString &p_text, uint p_options, bool p_forward = true);

    // If @p_cursor is not null, set the position of @p_cursor instead of current
    // cursor.
    bool findText(const QString &p_text, uint p_options, bool p_forward,
                  QTextCursor *p_cursor = NULL,
                  QTextCursor::MoveMode p_moveMode = QTextCursor::MoveAnchor);

    void replaceText(const QString &p_text, uint p_options,
                     const QString &p_replaceText, bool p_findNext);
    void replaceTextAll(const QString &p_text, uint p_options,
                        const QString &p_replaceText);

    // Clear SearchedKeyword highlight.
    void clearSearchedWordHighlight();

    // Clear SearchedKeywordUnderCursor Highlight.
    void clearSearchedWordUnderCursorHighlight(bool p_now = true);

    // Clear IncrementalSearchedKeyword highlight.
    void clearIncrementalSearchedWordHighlight(bool p_now = true);

    VFile *getFile() const;

    VEditConfig &getConfig();

    // Request to update Vim status.
    void requestUpdateVimStatus();

    QVariant inputMethodQuery(Qt::InputMethodQuery p_query) const Q_DECL_OVERRIDE;

    void setInputMethodEnabled(bool p_enabled);

    // Insert decoration markers or decorate selected text.
    void decorateText(TextDecoration p_decoration);

    // LineNumberArea will call this to request paint itself.
    void lineNumberAreaPaintEvent(QPaintEvent *p_event);

    // Scroll the content to make @p_block visible.
    // If the @p_block is too long to hold in one page, just let it occupy the
    // whole page.
    // Will not change current cursor.
    void makeBlockVisible(const QTextBlock &p_block);

    bool isBlockVisible(const QTextBlock &p_block);

    // Evaluate selected text or cursor word as magic words.
    void evaluateMagicWords();

signals:
    // Request VEditTab to save and exit edit mode.
    void saveAndRead();

    // Request VEditTab to discard and exit edit mode.
    void discardAndRead();

    // Request VEditTab to edit current note.
    void editNote();

    // Request VEditTab to save this file.
    void saveNote();

    // Emit when m_config has been updated.
    void configUpdated();

    // Emit when want to show message in status bar.
    void statusMessage(const QString &p_msg);

    // Emit when Vim status updated.
    void vimStatusUpdated(const VVim *p_vim);

    // Selection changed by mouse.
    void selectionChangedByMouse(bool p_hasSelection);

    // Request the edit tab to close find and replace dialog.
    void requestCloseFindReplaceDialog();

    // Emit when all initialization is ready.
    void ready();

public slots:
    virtual void highlightCurrentLine();

    // Jump to a title.
    // @p_forward: jump forward or backward.
    // @p_relativeLevel: 0 for the same level as current header;
    //                   negative value for upper level;
    //                   positive value is ignored.
    // Returns true if the jump succeeded.
    virtual bool jumpTitle(bool p_forward, int p_relativeLevel, int p_repeat);

private slots:
    void labelTimerTimeout();
    void highlightSelectedWord();
    void handleSaveExitAct();
    void handleDiscardExitAct();
    void handleEditAct();
    void highlightTrailingSpace();
    void handleCursorPositionChanged();

    // Update viewport margin to hold the line number area.
    void updateLineNumberAreaMargin();

    void updateLineNumberArea();

    // According to the document change, try to set the block line distance height
    // if affected blocks are not set.
    void updateBlockLineDistanceHeight(int p_pos, int p_charsRemoved, int p_charsAdded);

protected:
    QPointer<VFile> m_file;
    VEditOperations *m_editOps;

    VEditConfig m_config;

    virtual void updateFontAndPalette();
    virtual void contextMenuEvent(QContextMenuEvent *p_event) Q_DECL_OVERRIDE;

    // Used to implement dragging mouse with Ctrl and left button pressed to scroll.
    virtual void mousePressEvent(QMouseEvent *p_event) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *p_event) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent *p_event) Q_DECL_OVERRIDE;

    virtual void resizeEvent(QResizeEvent *p_event) Q_DECL_OVERRIDE;

    // Update m_config according to VConfigManager.
    void updateConfig();

    // Called in contextMenuEvent() to modify the context menu.
    virtual void alterContextMenu(QMenu *p_menu, const QList<QAction *> &p_actions);

    // Set read-only property and highlight current line.
    void setReadOnlyAndHighlight(bool p_readonly);

private:
    QLabel *m_wrapLabel;
    QTimer *m_labelTimer;

    // doHighlightExtraSelections() will highlight these selections.
    // Selections are indexed by SelectionId.
    QVector<QList<QTextEdit::ExtraSelection> > m_extraSelections;

    QColor m_selectedWordColor;
    QColor m_searchedWordColor;
    QColor m_searchedWordCursorColor;
    QColor m_incrementalSearchedWordColor;
    QColor m_trailingSpaceColor;

    // Timer for extra selections highlight.
    QTimer *m_highlightTimer;

    bool m_readyToScroll;
    bool m_mouseMoveScrolled;
    int m_oriMouseX;
    int m_oriMouseY;

    // Whether enable input method.
    bool m_enableInputMethod;

    LineNumberArea *m_lineNumberArea;

    void showWrapLabel();

    // Trigger the timer to request highlight.
    // If @p_now is true, stop the timer and highlight immediately.
    void highlightExtraSelections(bool p_now = false);

    // Do the real work to highlight extra selections.
    void doHighlightExtraSelections();

    // Find all the occurences of @p_text.
    QList<QTextCursor> findTextAll(const QString &p_text, uint p_options);

    // @p_fileter: a function to filter out highlight results.
    void highlightTextAll(const QString &p_text, uint p_options,
                          SelectionId p_id, QTextCharFormat p_format,
                          void (*p_filter)(VEdit *, QList<QTextEdit::ExtraSelection> &) = NULL);

    void highlightSearchedWord(const QString &p_text, uint p_options);

    // Highlight @p_cursor as the searched keyword under cursor.
    void highlightSearchedWordUnderCursor(const QTextCursor &p_cursor);

    // Highlight @p_cursor as the incremental searched keyword.
    void highlightIncrementalSearchedWord(const QTextCursor &p_cursor);

    bool wordInSearchedSelection(const QString &p_text);

    // Return the first visible block.
    QTextBlock firstVisibleBlock();

    // Return the y offset of the content.
    int contentOffsetY();

    // Find @p_text in the document starting from @p_start.
    // Returns true if @p_text is found and set @p_cursor to indicate
    // the position.
    // Will NOT change current cursor.
    bool findTextHelper(const QString &p_text, uint p_options,
                        bool p_forward, int p_start,
                        bool &p_wrapped, QTextCursor &p_cursor);
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(VEdit *p_editor)
        : QWidget(p_editor), m_editor(p_editor),
          m_document(p_editor->document()),
          m_width(0), m_blockCount(-1)
    {
        m_digitWidth = m_editor->fontMetrics().width(QLatin1Char('1'));
        m_digitHeight = m_editor->fontMetrics().height();
    }

    QSize sizeHint() const Q_DECL_OVERRIDE
    {
        return QSize(calculateWidth(), 0);
    }

    int calculateWidth() const;

    int getDigitHeight() const
    {
        return m_digitHeight;
    }

protected:
    void paintEvent(QPaintEvent *p_event) Q_DECL_OVERRIDE
    {
        m_editor->lineNumberAreaPaintEvent(p_event);
    }

private:
    VEdit *m_editor;
    const QTextDocument *m_document;
    int m_width;
    int m_blockCount;
    int m_digitWidth;
    int m_digitHeight;
};

#endif // VEDIT_H
