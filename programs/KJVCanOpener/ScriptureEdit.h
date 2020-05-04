/****************************************************************************
**
** Copyright (C) 2012-2020 Donna Whisnant, a.k.a. Dewtronics.
** Contact: http://www.dewtronics.com/
**
** This file is part of the KJVCanOpener Application as originally written
** and developed for Bethel Church, Festus, MO.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU General Public License
** version 3.0 as published by the Free Software Foundation and appearing
** in the file gpl-3.0.txt included in the packaging of this file. Please
** review the following information to ensure the GNU General Public License
** version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and
** Dewtronics.
**
****************************************************************************/

#ifndef SCRIPTUREEDIT_H
#define SCRIPTUREEDIT_H

#include "dbstruct.h"
#include "Highlighter.h"
#include "PhraseEdit.h"
#include "DelayedExecutionTimer.h"

#include <QWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTimer>
#include <QMenu>
#include <QPoint>
#include <QMimeData>
#include <QColor>
#include <QAction>
#include <QActionGroup>
#include <QUrl>

// ============================================================================

// Forward declarations:
class CKJVCanOpener;
class FindDialog;

// ============================================================================

//
// CScriptureText - Base template class functionality for CScriptureEdit and CScriptureBrowser
//
//
//	It really bugged me that CScriptureEdit and CScriptureBrowser were absolutely
//	identical except that one inherited from QTextEdit and the other QTextBrowser.
//	But Q_OBJECT doesn't work in templates, so I couldn't just make it a template
//	and inherit CScriptureText and CScriptureBrowser from that.  That would have
//	been too easy.
//
//	So, I turned the problem on its head and derived intermediate classes
//	(i_CScriptureEdit and i_CScriptureBrowser) inherited from QTextEdit and
//	QTextBrowser that do nothing but the Qt Signal/Slot mechanism with Q_OBJECT.
//	Then, inherit the final CScriptureEdit and CScriptureBrowser classes from this
//	CScriptureText template that gives the actual final desired functionality.
//	All because Q_OBJECT can't exist in templated classes!!
//

template <class T, class U>
class CScriptureText : public T
{
public:
	CScriptureText(CBibleDatabasePtr pBibleDatabase, QWidget *parent = nullptr);
	virtual ~CScriptureText();

	void savePersistentSettings(const QString &strGroup);
	void restorePersistentSettings(const QString &strGroup);

	CPhraseEditNavigator &navigator()
	{
		return m_navigator;
	}

	QMenu *getEditMenu() { return m_pEditMenu; }

	bool haveSelection() const {
		return (m_lstSelectedPhrases.haveSelection());
	}
	CSelectionPhraseTagList selection() const {
		if (m_lstSelectedPhrases.haveSelection()) return m_lstSelectedPhrases.selection();
		CSelectionPhraseTagList lstSelection;
		lstSelection.append(TPhraseTag(m_tagLast.relIndex(), 0));
		return lstSelection;
	}

	bool haveDetails() const;

	CKJVCanOpener *parentCanOpener() const;

	void displayCopyCompleteToolTip() const;

//signals:
//	void gotoIndex(const TPhraseTag &tag);
//	void activatedScriptureText();
//	void copyRawAvailable(bool bAvailable);
//	void copyVersesAvailable(bool bAvailable);

protected:
	virtual bool event(QEvent *ev);
	virtual bool eventFilter(QObject *obj, QEvent *ev);
	virtual void mouseDoubleClickEvent(QMouseEvent *ev);
	virtual QMimeData *createMimeDataFromSelection () const;
	virtual void mouseMoveEvent(QMouseEvent *ev);

protected:
	virtual void updateSelection();
	virtual void copyVersesCommon(bool bPlainOnly);

#ifdef TOUCH_GESTURE_PROCESSING
	virtual void grabGestures(bool bGrab = true);
#endif

//protected slots:
protected:
#ifdef USING_QT_SPEECH
	virtual void en_readSelection();			// Equivalent to SpeechPlay
	virtual void en_readFromCursor();
	virtual void setSpeechActionEnables();
#endif

	virtual void en_findParentCanOpener();
	virtual void en_findDialog();

	virtual void en_customContextMenuRequested(const QPoint &pos);
	virtual void en_cursorPositionChanged();
	virtual void en_selectionChanged();
	virtual void clearHighlighting();

	virtual void en_detailUpdate();

	virtual void en_changedBibleDatabaseSettings(const QString &strUUID, const TBibleDatabaseSettings &aSettings);

//public slots:
public:
	virtual void rerender();
	virtual void setFont(const QFont& aFont);
	virtual void setTextBrightness(bool bInvert, int nBrightness);
	virtual void showDetails();
	virtual void showPassageNavigator();
	virtual void en_copy();
	virtual void en_copyPlain();
	virtual void en_copyRaw();
	virtual void en_copyVeryRaw();
	virtual void en_copyVerses();
	virtual void en_copyVersesPlain();
	virtual void en_copyReferenceDetails();
	virtual void en_copyPassageStatistics();
	virtual void en_copyEntirePassageDetails();
	virtual void en_highlightPassage(int ndxHighlighterTool, bool bSecondaryActive);
	virtual void en_anchorClicked(const QUrl &link);
	virtual void en_showAllNotes();
	virtual void en_hideAllNotes();

	virtual void en_gotoIndex(const TPhraseTag &tag);

private:
	void setLastActiveTag();		// Sets last active tag from last tag if we're on an active verse/word

private:
	CBibleDatabasePtr m_pBibleDatabase;
	FindDialog *m_pFindDialog;
	bool m_bDoingPopup;				// True if popping up a menu or dialog and we don't want the highlight to disable
	bool m_bDoingSelectionChange;	// True if processing selection change to guard against reentracy
	CPhraseEditNavigator m_navigator;
	CCursorFollowHighlighter m_CursorFollowHighlighter;
	QTimer m_HighlightTimer;
	CRelIndex m_ndxCurrent;			// Current Displayed index as captured on received en_gotoIndex()
	TPhraseTag m_tagLast;			// Last mouse/keyboard reference tag for tool tips, etc (used for copying, etc)
	TPhraseTag m_tagLastActive;		// Last active position -- i.e. m_tagLast for a verse and/or word (as opposed to book/chapter
	CSelectedPhraseList m_lstSelectedPhrases;		// Selected phrases and cursor selection reference
	bool m_bDoPlainCopyOnly;		// Flag for the createMimeDataFromSelection function to use only plain text
	QPoint m_ptLastTrackPosition;	// Last Viewport mouse track position or Context Popup position for popups

	QMenu *m_pEditMenu;				// Edit menu for main screen when this editor is active
	// ----
	QAction *m_pActionCopy;			// Edit menu copy (text as shown)
	QAction *m_pActionCopyPlain;	// Edit menu copy, as shown, but plaintext
	QAction *m_pActionCopyRaw;		// Edit menu copy raw phrase text
	QAction *m_pActionCopyVeryRaw;	// Edit menu copy very (no punctuation) raw phrase text
	// ----
	QAction *m_pActionCopyVerses;	// Edit menu copy as formatted verses
	QAction *m_pActionCopyVersesPlain;	// Edit menu copy as formatted verses, but plaintext
	// ----
	QAction *m_pActionCopyReferenceDetails;			// Edit menu, Reference ToolTip Copy
	QAction *m_pActionCopyPassageStatistics;		// Edit menu, Statistics ToolTip Copy
	QAction *m_pActionCopyEntirePassageDetails;		// Edit menu, Entire ToolTip Copy
	// ----
	QAction *m_pActionSelectAll;	// Edit menu select all
	// ----
	QAction *m_pActionFind;			// Edit menu Find
	QAction *m_pActionFindNext;		// Edit menu Find Next
	QAction *m_pActionFindPrev;		// Edit menu Find Previous
	// ----
	QAction *m_pActionShowAllNotes;	// Edit menu Show All Notes
	QAction *m_pActionHideAllNotes;	// Edit menu Hide All Notes
	// ----
	QAction *m_pStatusAction;		// Used to update the status bar without an enter/leave sequence
	// ----
	mutable CKJVCanOpener *m_pParentCanOpener;	// Parent CanOpener once we locate it.  Set lazily on demand since parent doesn't exist yet during object creation

	DelayedExecutionTimer m_dlyDetailUpdate;
	DelayedExecutionTimer m_dlyRerenderCompressor;

#define begin_popup()							\
			bool bPopupSave = m_bDoingPopup;	\
			m_bDoingPopup = true;
#define end_popup()								\
			m_bDoingPopup = bPopupSave;

};


// ============================================================================

// Intermediate classes to marshall signals/slots:

class i_CScriptureEdit : public QTextEdit
{
	Q_OBJECT
public:
	explicit i_CScriptureEdit(QWidget *parent = nullptr)
		:	QTextEdit(parent)
	{ }

	static bool useToolTipEdit() { return true; }
	static bool useFindDialog() { return false; }

signals:
	void gotoIndex(const TPhraseTag &tag);
	void activatedScriptureText();
	void copyRawAvailable(bool bAvailable);
	void copyVersesAvailable(bool bAvailable);

protected slots:
#ifdef USING_QT_SPEECH
	virtual void en_readSelection() = 0;
	virtual void en_readFromCursor() = 0;
	virtual void setSpeechActionEnables() = 0;
#endif

	virtual void en_findParentCanOpener() = 0;
	virtual void en_findDialog() = 0;

	virtual void en_customContextMenuRequested(const QPoint &pos) = 0;
	virtual void en_cursorPositionChanged() = 0;
	virtual void en_selectionChanged() = 0;
	virtual void clearHighlighting() = 0;

	virtual void en_detailUpdate() = 0;

	virtual void en_changedBibleDatabaseSettings(const QString &strUUID, const TBibleDatabaseSettings &aSettings) = 0;

public slots:
	virtual void rerender() = 0;
	virtual void setFont(const QFont& aFont) = 0;
	virtual void setTextBrightness(bool bInvert, int nBrightness) = 0;
	virtual void showDetails() = 0;
//	virtual void showPassageNavigator() = 0;			-- Don't implement this because we don't want the navigator launching the navigator
	virtual void en_copy() = 0;
	virtual void en_copyPlain() = 0;
	virtual void en_copyRaw() = 0;
	virtual void en_copyVeryRaw() = 0;
	virtual void en_copyVerses() = 0;
	virtual void en_copyVersesPlain() = 0;
	virtual void en_copyReferenceDetails() = 0;
	virtual void en_copyPassageStatistics() = 0;
	virtual void en_copyEntirePassageDetails() = 0;
	virtual void en_highlightPassage(int ndxHighlighterTool, bool bSecondaryActive) = 0;
	virtual void en_anchorClicked(const QUrl &link) = 0;
	virtual void en_showAllNotes() = 0;
	virtual void en_hideAllNotes() = 0;

	virtual void en_gotoIndex(const TPhraseTag &tag) = 0;
};

class i_CScriptureBrowser : public QTextBrowser
{
	Q_OBJECT
public:
	explicit i_CScriptureBrowser(QWidget *parent = nullptr)
		:	QTextBrowser(parent)
	{ }

	static bool useToolTipEdit() { return true; }
	static bool useFindDialog() { return true; }

signals:
	void gotoIndex(const TPhraseTag &tag);
	void activatedScriptureText();
	void copyRawAvailable(bool bAvailable);
	void copyVersesAvailable(bool bAvailable);

protected slots:
#ifdef USING_QT_SPEECH
	virtual void en_readSelection() = 0;
	virtual void en_readFromCursor() = 0;
	virtual void setSpeechActionEnables() = 0;
#endif

	virtual void en_findParentCanOpener() = 0;
	virtual void en_findDialog() = 0;

	virtual void en_customContextMenuRequested(const QPoint &pos) = 0;
	virtual void en_cursorPositionChanged() = 0;
	virtual void en_selectionChanged() = 0;
	virtual void clearHighlighting() = 0;

	virtual void en_detailUpdate() = 0;

	virtual void en_changedBibleDatabaseSettings(const QString &strUUID, const TBibleDatabaseSettings &aSettings) = 0;

public slots:
	virtual void rerender() = 0;
	virtual void setFont(const QFont& aFont) = 0;
	virtual void setTextBrightness(bool bInvert, int nBrightness) = 0;
	virtual void showDetails() = 0;
	virtual void showPassageNavigator() = 0;
	virtual void en_copy() = 0;
	virtual void en_copyPlain() = 0;
	virtual void en_copyRaw() = 0;
	virtual void en_copyVeryRaw() = 0;
	virtual void en_copyVerses() = 0;
	virtual void en_copyVersesPlain() = 0;
	virtual void en_copyReferenceDetails() = 0;
	virtual void en_copyPassageStatistics() = 0;
	virtual void en_copyEntirePassageDetails() = 0;
	virtual void en_highlightPassage(int ndxHighlighterTool, bool bSecondaryActive) = 0;
	virtual void en_anchorClicked(const QUrl &link) = 0;
	virtual void en_showAllNotes() = 0;
	virtual void en_hideAllNotes() = 0;

	virtual void en_gotoIndex(const TPhraseTag &tag) = 0;
};

// ============================================================================

// Real Exported Classes to serve as our subclassed controls:

class CScriptureEdit : public CScriptureText<i_CScriptureEdit, QTextEdit>
{
public:
	explicit CScriptureEdit(CBibleDatabasePtr pBibleDatabase, QWidget *parent = nullptr)
		:	CScriptureText<i_CScriptureEdit, QTextEdit>(pBibleDatabase, parent)
	{ }
};

class CScriptureBrowser : public CScriptureText<i_CScriptureBrowser, QTextBrowser>
{
public:
	explicit CScriptureBrowser(CBibleDatabasePtr pBibleDatabase, QWidget *parent = nullptr)
		:	CScriptureText<i_CScriptureBrowser, QTextBrowser>(pBibleDatabase, parent)
	{ }
};

// ============================================================================

#endif // SCRIPTUREEDIT_H
