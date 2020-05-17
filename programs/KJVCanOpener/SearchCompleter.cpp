/****************************************************************************
**
** Copyright (C) 2013-2020 Donna Whisnant, a.k.a. Dewtronics.
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

#include "SearchCompleter.h"
#include "PhraseEdit.h"
#include "ParseSymbols.h"
#include "BusyCursor.h"

#include <QString>
#include <QStringRef>
#include <QRegExp>
#include <QTimer>

#ifdef QT_WIDGETS_LIB
#include <QAbstractItemView>
#endif

#include <assert.h>

#include <algorithm>

#if QT_VERSION < 0x050000
#include <QInputContext>
#endif

// ============================================================================

class TBasicWordHelper
{
public:
	TBasicWordHelper(const TBasicWordList &aWordList)
		:	m_lstBasicWords(aWordList)
	{

	}

	int indexOf_renderedWord(const QString &strWord, int nFrom = 0) const
	{
		if (nFrom < 0)
			nFrom = qMax(nFrom + m_lstBasicWords.size(), 0);
		if (nFrom < m_lstBasicWords.size()) {
			for (int ndx = nFrom; ndx < m_lstBasicWords.size(); ++ndx) {
				if (m_lstBasicWords.at(ndx)->renderedWord() == strWord) return ndx;
			}
		}
		return -1;
	}

	int lastIndexOf_renderedWord(const QString &strWord, int nFrom = -1) const
	{
		if (nFrom < 0)
			nFrom += m_lstBasicWords.size();
		else if (nFrom >= m_lstBasicWords.size())
			nFrom = m_lstBasicWords.size()-1;
		if (nFrom >= 0) {
			for (int ndx = nFrom; ndx >= 0; --ndx) {
				if (m_lstBasicWords.at(ndx)->renderedWord() == strWord) return ndx;
			}
		}
		return -1;
	}

	int indexOf_decomposedWord(const QString &strWord, int nFrom = 0) const
	{
		if (nFrom < 0)
			nFrom = qMax(nFrom + m_lstBasicWords.size(), 0);
		if (nFrom < m_lstBasicWords.size()) {
			for (int ndx = nFrom; ndx < m_lstBasicWords.size(); ++ndx) {
				if (m_lstBasicWords.at(ndx)->decomposedWord() == strWord) return ndx;
			}
		}
		return -1;
	}

	int lastIndexOf_decomposedWord(const QString &strWord, int nFrom = -1) const
	{
		if (nFrom < 0)
			nFrom += m_lstBasicWords.size();
		else if (nFrom >= m_lstBasicWords.size())
			nFrom = m_lstBasicWords.size()-1;
		if (nFrom >= 0) {
			for (int ndx = nFrom; ndx >= 0; --ndx) {
				if (m_lstBasicWords.at(ndx)->decomposedWord() == strWord) return ndx;
			}
		}
		return -1;
	}

	int indexOf_renderedWord(const QRegExp &rx, int nFrom = 0) const
	{
		if (nFrom < 0)
			nFrom = qMax(nFrom + m_lstBasicWords.size(), 0);
		for (int i = nFrom; i < m_lstBasicWords.size(); ++i) {
			if (rx.exactMatch(m_lstBasicWords.at(i)->renderedWord()))
				return i;
		}
		return -1;
	}

	int lastIndexOf_renderedWord(const QRegExp &rx, int nFrom = -1) const
	{
		if (nFrom < 0)
			nFrom += m_lstBasicWords.size();
		else if (nFrom >= m_lstBasicWords.size())
			nFrom = m_lstBasicWords.size() - 1;
		for (int i = nFrom; i >= 0; --i) {
			if (rx.exactMatch(m_lstBasicWords.at(i)->renderedWord()))
				return i;
			}
		return -1;
	}

	int indexOf_decomposedWord(const QRegExp &rx, int nFrom = 0) const
	{
		if (nFrom < 0)
			nFrom = qMax(nFrom + m_lstBasicWords.size(), 0);
		for (int i = nFrom; i < m_lstBasicWords.size(); ++i) {
			if (rx.exactMatch(m_lstBasicWords.at(i)->decomposedWord()))
				return i;
		}
		return -1;
	}

	int lastIndexOf_decomposedWord(const QRegExp &rx, int nFrom = -1) const
	{
		if (nFrom < 0)
			nFrom += m_lstBasicWords.size();
		else if (nFrom >= m_lstBasicWords.size())
			nFrom = m_lstBasicWords.size() - 1;
		for (int i = nFrom; i >= 0; --i) {
			if (rx.exactMatch(m_lstBasicWords.at(i)->decomposedWord()))
				return i;
			}
		return -1;
	}

private:
	const TBasicWordList &m_lstBasicWords;
};

// ============================================================================

QString CSearchStringListModel::decompose(const QString &strWord, bool bRemoveHyphens)
{
	QString strDecomposed = deApostrHyphen(strWord, bRemoveHyphens).normalized(QString::NormalizationForm_KD);

	strDecomposed.replace(QChar(0x00C6), "Ae");				// U+00C6	&#198;		AE character
	strDecomposed.replace(QChar(0x00E6), "ae");				// U+00E6	&#230;		ae character
	strDecomposed.replace(QChar(0x0132), "IJ");				// U+0132	&#306;		IJ character
	strDecomposed.replace(QChar(0x0133), "ij");				// U+0133	&#307;		ij character
	strDecomposed.replace(QChar(0x0152), "Oe");				// U+0152	&#338;		OE character
	strDecomposed.replace(QChar(0x0153), "oe");				// U+0153	&#339;		oe character

	// There are two possible ways to remove accent marks:
	//
	//		1) strDecomposed.remove(QRegExp("[^a-zA-Z\\s]"));
	//
	//		2) Remove characters of class "Mark" (QChar::Mark_NonSpacing,
	//				QChar::Mark_SpacingCombining, QChar::Mark_Enclosing),
	//				which can be done by checking isMark()
	//

	for (int nPos = strDecomposed.size()-1; nPos >= 0; --nPos) {
		if (strDecomposed.at(nPos).isMark()) strDecomposed.remove(nPos, 1);
	}

	return strDecomposed;
}

QString CSearchStringListModel::deApostrHyphen(const QString &strWord, bool bRemoveHyphens)
{
	return deHyphen(deApostrophe(strWord, false), bRemoveHyphens);
}

QString CSearchStringListModel::deApostrophe(const QString &strWord, bool bRemove)
{
	static const QString strApostropheRegExp = QChar('[') + QRegExp::escape(g_strApostrophes) + QChar(']');
	static const QRegExp expApostrophe(strApostropheRegExp);

	QString strDecomposed = strWord;

	if (!bRemove) {
		strDecomposed.replace(expApostrophe, "'");
	} else {
		strDecomposed.remove(expApostrophe);
	}

	return strDecomposed;
}

QString CSearchStringListModel::deHyphen(const QString &strWord, bool bRemove)
{
	static const QString strHyphenRegExp = QChar('[') + QRegExp::escape(g_strHyphens) + QChar(']');
	static const QRegExp expHyphen(strHyphenRegExp);

	QString strDecomposed;

	if (!bRemove) {
		strDecomposed = strWord;
		strDecomposed.replace(expHyphen, "-");
	} else {
		// Remove hyphens, but leave embedded regexp charsets intact:
		int nPos = 0;
		QString strDecomposed2 = strWord;
		while (!strDecomposed2.isEmpty()) {
			nPos = strDecomposed2.indexOf(QChar('['));
			if (nPos != -1) {
				strDecomposed += strDecomposed2.mid(0, nPos).remove(expHyphen);
				strDecomposed2 = strDecomposed2.mid(nPos);
				nPos = strDecomposed2.indexOf(QChar(']'));
				if (nPos != -1) {
					strDecomposed += strDecomposed2.mid(0, nPos+1);
					strDecomposed2 = strDecomposed2.mid(nPos+1);
				} else {
					strDecomposed += strDecomposed2;
					strDecomposed2.clear();
				}
			} else {
				strDecomposed += strDecomposed2.remove(expHyphen);
				strDecomposed2.clear();
			}
		}
	}

	return strDecomposed;
}

// ============================================================================

CSearchParsedPhraseListModel::CSearchParsedPhraseListModel(const CParsedPhrase &parsedPhrase, QObject *parent)
	:	CSearchStringListModel(parent),
		m_parsedPhrase(parsedPhrase),
		m_nCursorWord(-1)			// Force initial update
{

}

CSearchParsedPhraseListModel::~CSearchParsedPhraseListModel()
{

}

int CSearchParsedPhraseListModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return m_parsedPhrase.nextWordsList().size();
}

QVariant CSearchParsedPhraseListModel::data(const QModelIndex &index, int role) const
{
	if ((index.row() < 0) || (index.row() >= m_parsedPhrase.nextWordsList().size()))
		return QVariant();

	if (role == Qt::DisplayRole)
		return (m_parsedPhrase.nextWordsList().at(index.row()).renderedWord());

	if (role == Qt::EditRole)
		return m_parsedPhrase.nextWordsList().at(index.row()).decomposedWord();

	if (role == SOUNDEX_ENTRY_ROLE)
		return m_parsedPhrase.bibleDatabase()->soundEx(m_parsedPhrase.nextWordsList().at(index.row()).decomposedWord());

	return QVariant();
}

bool CSearchParsedPhraseListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	Q_UNUSED(index);
	Q_UNUSED(value);
	Q_UNUSED(role);

	return false;
}

QString CSearchParsedPhraseListModel::soundEx(const QString &strDecomposedWord, bool bCache) const
{
	return m_parsedPhrase.bibleDatabase()->soundEx(strDecomposedWord, bCache);
}

QString CSearchParsedPhraseListModel::cursorWord() const
{
	return m_parsedPhrase.GetCursorWord();
}

void CSearchParsedPhraseListModel::setWordsFromPhrase(bool bForceUpdate)
{
#ifdef SEARCH_COMPLETER_DEBUG_OUTPUT
	qDebug("SearchStringListModel::setWordsFromPhrase : %d  OldCursorPos: %d", m_parsedPhrase.GetCursorWordPos(), m_nCursorWord);
#endif

	if ((m_parsedPhrase.GetCursorWordPos() != m_nCursorWord) || (bForceUpdate)) {
		m_nCursorWord = m_parsedPhrase.GetCursorWordPos();

		emit beginResetModel();

//		m_ParsedPhrase.nextWordsList();

		m_lstBasicWords.clear();
		m_lstBasicWords.reserve(m_parsedPhrase.nextWordsList().size());
		for (int ndx = 0; ndx < m_parsedPhrase.nextWordsList().size(); ++ndx) {
			m_lstBasicWords.append(&m_parsedPhrase.nextWordsList().at(ndx));
		}

		emit endResetModel();

		emit modelChanged();

		// Free our list since it's only valid immediately after this function runs anyway:
		m_lstBasicWords.clear();

	}
}

// ============================================================================

#ifdef QT_WIDGETS_LIB

CSearchDictionaryListModel::CSearchDictionaryListModel(CDictionaryDatabasePtr pDictionary, const QTextEdit &editorWord, QObject *parent)
	:	CSearchStringListModel(parent),
		m_pDictionaryDatabase(pDictionary),
		m_editorWord(editorWord)
{
	assert(!pDictionary.isNull());

	m_lstBasicWords.clear();
	m_lstBasicWords.reserve(m_pDictionaryDatabase->wordCount());
	for (int ndx = 0; ndx < m_pDictionaryDatabase->wordCount(); ++ndx) {
		m_lstBasicWords.append(&m_pDictionaryDatabase->wordDefinitionsEntry(m_pDictionaryDatabase->wordEntry(ndx)));
	}
}

CSearchDictionaryListModel::~CSearchDictionaryListModel()
{

}

int CSearchDictionaryListModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return m_pDictionaryDatabase->wordCount();
}

QVariant CSearchDictionaryListModel::data(const QModelIndex &index, int role) const
{
	if ((index.row() < 0) || (index.row() >= m_pDictionaryDatabase->wordCount()))
		return QVariant();

	if (role == Qt::DisplayRole)
		return m_pDictionaryDatabase->wordDefinitionsEntry(m_pDictionaryDatabase->wordEntry(index.row())).renderedWord();

	if (role == Qt::EditRole)
		return m_pDictionaryDatabase->wordDefinitionsEntry(m_pDictionaryDatabase->wordEntry(index.row())).decomposedWord();

	if (role == SOUNDEX_ENTRY_ROLE)
		return m_pDictionaryDatabase->soundEx(m_pDictionaryDatabase->wordEntry(index.row()));

	return QVariant();
}

bool CSearchDictionaryListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	Q_UNUSED(index);
	Q_UNUSED(value);
	Q_UNUSED(role);

	return false;
}

QString CSearchDictionaryListModel::soundEx(const QString &strDecomposedWord, bool bCache) const
{
	return m_pDictionaryDatabase->soundEx(strDecomposedWord, bCache);
}

QString CSearchDictionaryListModel::cursorWord() const
{
	return m_editorWord.toPlainText();
}

void CSearchDictionaryListModel::setWordsFromPhrase(bool bForceUpdate)
{
	Q_UNUSED(bForceUpdate)

//	emit beginResetModel();
//	emit endResetModel();
//	emit modelChanged();
}

#endif

// ============================================================================

#ifdef QT_WIDGETS_LIB

CSearchStrongsDictionaryListModel::CSearchStrongsDictionaryListModel(CDictionaryDatabasePtr pDictionary, const QTextEdit &editorWord, QObject *parent)
	:	QAbstractListModel(parent),
		m_pDictionaryDatabase(pDictionary),
		m_editorWord(editorWord)
{
	assert(!pDictionary.isNull());
}

CSearchStrongsDictionaryListModel::~CSearchStrongsDictionaryListModel()
{
}

int CSearchStrongsDictionaryListModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return m_pDictionaryDatabase->wordCount();
}

QVariant CSearchStrongsDictionaryListModel::data(const QModelIndex &index, int role) const
{
	if ((index.row() < 0) || (index.row() >= m_pDictionaryDatabase->wordCount()))
		return QVariant();

	if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
		return m_pDictionaryDatabase->wordEntry(index.row());

	return QVariant();
}

QString CSearchStrongsDictionaryListModel::cursorWord() const
{
	return m_editorWord.toPlainText();
}

#endif

// ============================================================================

#ifdef QT_WIDGETS_LIB

CSearchCompleter::CSearchCompleter(const CParsedPhrase &parsedPhrase, QWidget *parentWidget)
	:	QCompleter(parentWidget),
		m_nCompletionFilterMode(SCFME_NORMAL),
		m_pSearchStringListModel(nullptr),
		m_pSoundExFilterModel(nullptr)
{
	m_pSearchStringListModel = new CSearchParsedPhraseListModel(parsedPhrase, this);
	m_pSoundExFilterModel = new CSoundExSearchCompleterFilter(m_pSearchStringListModel, this);

	setWidget(parentWidget);
	setCaseSensitivity(Qt::CaseInsensitive);
	// Note: CompletionMode, CompletionRole, and ModelSorting properties are set in setCompletionFilterMode(), as they depend on the mode:
	setCompletionFilterMode(m_nCompletionFilterMode);
	setModel(m_pSoundExFilterModel);
}

CSearchCompleter::CSearchCompleter(CDictionaryDatabasePtr pDictionary, const QTextEdit &editorWord, QWidget *parentWidget)
	:	QCompleter(parentWidget),
		m_nCompletionFilterMode(SCFME_NORMAL),
		m_pSearchStringListModel(nullptr),
		m_pSoundExFilterModel(nullptr)
{
	m_pSearchStringListModel = new CSearchDictionaryListModel(pDictionary, editorWord, this);
	m_pSoundExFilterModel = new CSoundExSearchCompleterFilter(m_pSearchStringListModel, this);

	setWidget(parentWidget);
	setCaseSensitivity(Qt::CaseInsensitive);
	// Note: CompletionMode, CompletionRole, and ModelSorting properties are set in setCompletionFilterMode(), as they depend on the mode:
	setCompletionFilterMode(m_nCompletionFilterMode);
	setModel(m_pSoundExFilterModel);
}

CSearchCompleter::CSearchCompleter(QWidget *parentWidget)
	:	QCompleter(parentWidget),
		m_nCompletionFilterMode(SCFME_NORMAL),
		m_pSearchStringListModel(nullptr),
		m_pSoundExFilterModel(nullptr)
{
	setWidget(parentWidget);
	setCaseSensitivity(Qt::CaseInsensitive);
}

CSearchCompleter::~CSearchCompleter()
{

}

void CSearchCompleter::setCompletionFilterMode(SEARCH_COMPLETION_FILTER_MODE_ENUM nCompletionFilterMode)
{
	assert(m_pSoundExFilterModel != nullptr);

	switch (nCompletionFilterMode) {
		case SCFME_NORMAL:
			setCompletionMode(QCompleter::PopupCompletion);
			m_pSoundExFilterModel->setSoundExEnabled(false);
			setCompletionRole(Qt::EditRole);
			setModelSorting(QCompleter::CaseInsensitivelySortedModel);
			break;
		case SCFME_UNFILTERED:
			setCompletionMode(QCompleter::UnfilteredPopupCompletion);
			m_pSoundExFilterModel->setSoundExEnabled(false);
			setCompletionRole(Qt::DisplayRole);
			setModelSorting(QCompleter::UnsortedModel);				// We're sorted by the editRole, not the displayRole
			break;
		case SCFME_SOUNDEX:
			setCompletionMode(QCompleter::UnfilteredPopupCompletion);
			m_pSoundExFilterModel->setSoundExEnabled(true);
			setCompletionRole(Qt::DisplayRole);
			setModelSorting(QCompleter::UnsortedModel);				// We're sorted by the editRole, not the displayRole
			break;
	}

	m_nCompletionFilterMode = nCompletionFilterMode;
	if (!m_pSearchStringListModel->isDynamicModel()) {
		QTimer::singleShot(1, m_pSoundExFilterModel, SLOT(en_modelChanged()));		// Force a delayed update for models that don't do auto per-word updates (i.e. static models)
	}
}

void CSearchCompleter::setFilterMatchString()
{
	QString strPrefix = m_pSearchStringListModel->cursorWord();
	int nPreRegExp = strPrefix.indexOf(QRegExp("[\\[\\]\\*\\?]"));
	if (nPreRegExp != -1) strPrefix = strPrefix.left(nPreRegExp);
	QString strPrefixDecomposed = CSearchStringListModel::decompose(strPrefix, true);

#ifdef SEARCH_COMPLETER_DEBUG_OUTPUT
	qDebug("SearchCompleter::setFilterMatchString : %s", strPrefix.toUtf8().data());
#endif

	m_strFilterMatchString = strPrefix;
	setCompletionPrefix((completionFilterMode() == SCFME_NORMAL) ? strPrefixDecomposed : strPrefix);
	m_pSoundExFilterModel->setFilterFixedString(strPrefix);
}

void CSearchCompleter::selectFirstMatchString()
{
#ifdef SEARCH_COMPLETER_DEBUG_OUTPUT
	qDebug("SelectFirstMatch: CursorWord: \"%s\"  CurrentCompletion: \"%s\"", m_pSearchStringListModel->cursorWord().toUtf8().data(), currentCompletion().toUtf8().data());
	qDebug("Completion Model Size: %d", completionModel()->rowCount());
#endif

	popup()->clearSelection();

	QModelIndex indexFirstComposedWord = soundExFilterModel()->firstMatchStringIndex(true);
	QModelIndex indexFirstDecomposedWord = soundExFilterModel()->firstMatchStringIndex(false);

	switch (completionFilterMode()) {
		case SCFME_NORMAL:
		case SCFME_SOUNDEX:
			if (indexFirstComposedWord.isValid()) {
				int nCompCount = completionModel()->rowCount();
				for (int nComp = 0; nComp < nCompCount; ++nComp) {
					QModelIndex ndxComp = completionModel()->index(nComp, 0);
					if (ndxComp.data(Qt::DisplayRole).toString().compare(indexFirstComposedWord.data(Qt::DisplayRole).toString()) == 0) {
						popup()->setCurrentIndex(ndxComp);
						popup()->selectionModel()->select(ndxComp, QItemSelectionModel::Select);
						break;
					}
				}
			} else if (indexFirstDecomposedWord.isValid()) {
				int nCompCount = completionModel()->rowCount();
				for (int nComp = 0; nComp < nCompCount; ++nComp) {
					QModelIndex ndxComp = completionModel()->index(nComp, 0);
					if (ndxComp.data(Qt::EditRole).toString().compare(indexFirstDecomposedWord.data(Qt::EditRole).toString()) == 0) {
						if (completionFilterMode() == SCFME_SOUNDEX) {
							setCompletionPrefix(indexFirstDecomposedWord.data(Qt::DisplayRole).toString());		// Force assert a selection prefix or else it won't select it in this mode
						}
						popup()->setCurrentIndex(ndxComp);
						popup()->selectionModel()->select(ndxComp, QItemSelectionModel::Select);
						break;
					}
				}
			}
			break;
		case SCFME_UNFILTERED:
			if (indexFirstComposedWord.isValid()) {
				popup()->setCurrentIndex(indexFirstComposedWord);
			} else if (indexFirstDecomposedWord.isValid()) {
				int nCompCount = completionModel()->rowCount();
				for (int nComp = 0; nComp < nCompCount; ++nComp) {
					QModelIndex ndxComp = completionModel()->index(nComp, 0);
					if (ndxComp.data(Qt::EditRole).toString().compare(indexFirstDecomposedWord.data(Qt::EditRole).toString()) == 0) {
						setCompletionPrefix(indexFirstDecomposedWord.data(Qt::DisplayRole).toString());		// Force assert a selection prefix or else it won't select it in this mode
						popup()->setCurrentIndex(ndxComp);
						popup()->selectionModel()->select(ndxComp, QItemSelectionModel::Select);
						break;
					}
				}
			}
			break;
	}
}

void CSearchCompleter::setWordsFromPhrase(bool bForceUpdate)
{
	assert(m_pSearchStringListModel != nullptr);
	m_pSearchStringListModel->setWordsFromPhrase(bForceUpdate);
}

#endif

// ============================================================================


// ============================================================================

CSoundExSearchCompleterFilter::CSoundExSearchCompleterFilter(CSearchStringListModel *pSearchStringListModel, QObject *parent)
	:	QAbstractItemModel(parent),
		m_bSoundExEnabled(true),
		m_nFirstComposedMatchStringIndex(-1),
		m_nFirstDecomposedMatchStringIndex(-1),
		m_pSearchStringListModel(pSearchStringListModel)
{
	assert(m_pSearchStringListModel != nullptr);
	connect(m_pSearchStringListModel, SIGNAL(modelChanged()), this, SLOT(en_modelChanged()), Qt::DirectConnection);

	connect(m_pSearchStringListModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(en_dataChanged(const QModelIndex &, const QModelIndex &)));
	connect(m_pSearchStringListModel, SIGNAL(layoutAboutToBeChanged()), this, SIGNAL(layoutAboutToBeChanged()));
	connect(m_pSearchStringListModel, SIGNAL(layoutChanged()), this, SIGNAL(layoutChanged()));
}

CSoundExSearchCompleterFilter::~CSoundExSearchCompleterFilter()
{

}

int CSoundExSearchCompleterFilter::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return ((m_bSoundExEnabled && !m_strFilterFixedString.isEmpty()) ? m_lstMatchedIndexes.size() : m_pSearchStringListModel->rowCount()) ;
}

int CSoundExSearchCompleterFilter::columnCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : 1;
}

bool CSoundExSearchCompleterFilter::hasChildren(const QModelIndex & parent) const
{
	return parent.isValid() ? false : (rowCount() > 0);
}

QModelIndex CSoundExSearchCompleterFilter::parent(const QModelIndex & index) const
{
	Q_UNUSED(index);
	return QModelIndex();
}

QModelIndex	CSoundExSearchCompleterFilter::index(int row, int column, const QModelIndex & parent) const
{
	return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

QModelIndex CSoundExSearchCompleterFilter::mapFromSource(const QModelIndex &sourceIndex) const
{
	if (!sourceIndex.isValid()) return sourceIndex;

	if (sourceIndex.column() == 0) {
		for (int nRow = 0; nRow < m_lstMatchedIndexes.size(); ++nRow) {
			if (m_lstMatchedIndexes.at(nRow) == sourceIndex.row()) return createIndex(nRow, 0);
		}
	}
	return QModelIndex();
}

QModelIndex CSoundExSearchCompleterFilter::mapToSource(const QModelIndex &proxyIndex) const
{
	if (!proxyIndex.isValid()) return proxyIndex;

	if ((proxyIndex.column() == 0) && (proxyIndex.row() >= 0) && (proxyIndex.row() < m_lstMatchedIndexes.size())) {
//		return createIndex(m_lstMatchedIndexes.at(proxyIndex.row()), 0);
		return m_pSearchStringListModel->index(proxyIndex.row());
	}
	return QModelIndex();
}

QVariant CSoundExSearchCompleterFilter::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) return QVariant();
	if (m_strFilterFixedString.isEmpty() || !m_bSoundExEnabled) {
		return m_pSearchStringListModel->data(m_pSearchStringListModel->index(index.row()), role);
	}
	return m_pSearchStringListModel->data(m_pSearchStringListModel->index(m_lstMatchedIndexes.at(index.row())), role);
}

bool CSoundExSearchCompleterFilter::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) return false;
	if (m_strFilterFixedString.isEmpty() || !m_bSoundExEnabled) {
		return m_pSearchStringListModel->setData(m_pSearchStringListModel->index(index.row()), value, role);
	}
	return m_pSearchStringListModel->setData(m_pSearchStringListModel->index(m_lstMatchedIndexes.at(index.row())), value, role);
}

void CSoundExSearchCompleterFilter::setFilterFixedString(const QString &strPattern)
{
#ifdef SEARCH_COMPLETER_DEBUG_OUTPUT
	qDebug("SoundExSearchCompleter::setFilterFixedString : \"%s\" => \"%s\"", m_strFilterFixedString.toUtf8().data(), strPattern.toUtf8().data());
#endif

	bool bNeedUpdate = (m_strFilterFixedString.compare(strPattern) != 0);
	m_strFilterFixedString = strPattern;
	if (bNeedUpdate) {
		if (m_pSearchStringListModel->isDynamicModel()) {
			// This automatically updates our model:
			m_pSearchStringListModel->setWordsFromPhrase(true);		// Must force a rebuild on our dynamic models so our indexes will be correct
		} else {
			updateModel(m_bSoundExEnabled);		// No need to update our model if SoundEx isn't enabled -- the SearchListModel's reset will cause a modelChange that will update it
		}
	}
}

QModelIndex CSoundExSearchCompleterFilter::firstMatchStringIndex(bool bComposed) const
{
	int nIndex = (bComposed ? m_nFirstComposedMatchStringIndex : m_nFirstDecomposedMatchStringIndex);

	if (nIndex != -1) {
		if (m_strFilterFixedString.isEmpty() || !m_bSoundExEnabled) {
			return m_pSearchStringListModel->index(nIndex, 0);
		} else {
			return index(nIndex, 0);
		}
	}
	return QModelIndex();
}

void CSoundExSearchCompleterFilter::en_modelChanged()
{
#ifdef SEARCH_COMPLETER_DEBUG_OUTPUT
	qDebug("SoundExSearchCompleter::modelChanged");
#endif

	CBusyCursor iAmBusy(nullptr);

	int nCount = m_pSearchStringListModel->rowCount();
	m_mapSoundEx.clear();
	for (int nRow = 0; nRow < nCount; ++nRow) {
		QModelIndex ndx = m_pSearchStringListModel->index(nRow);
		if (m_bSoundExEnabled) m_mapSoundEx[ndx.data(CSearchStringListModel::SOUNDEX_ENTRY_ROLE).toString()].append(nRow);
	}

	updateModel(true);					// Always reset our model when base model resets
}

void CSoundExSearchCompleterFilter::updateModel(bool bResetModel)
{
#ifdef SEARCH_COMPLETER_DEBUG_OUTPUT
	qDebug("SoundExSearchCompleter::updateModel");
#endif

	if (bResetModel) beginResetModel();

	// Note: For Dynamic Models, like Search Phrases, this function can ONLY be called
	//		immediately after setWordsFromPhrase() is called on the SearchStringListModel,
	//		or from the en_modelChanged() slot triggered by the actual SearchStringListModel's
	//		setWordsFromPhrase() function, as the BasicWordsList is only valid immediately
	//		after the model is updated, since it contains references and pointers to
	//		transient data in order to prevent extraneous copying of strings.  On
	//		static models, like the Dictionary, it's safe to call it as needed:

	TBasicWordHelper lstBasicWords(m_pSearchStringListModel->basicWordsList());

	m_lstMatchedIndexes.clear();
	m_nFirstComposedMatchStringIndex = -1;
	m_nFirstDecomposedMatchStringIndex = -1;
	QString strDecomposedFilterString = CSearchStringListModel::decompose(m_strFilterFixedString, true);
	if (!m_strFilterFixedString.isEmpty()) {
		QRegExp expPrefix(strDecomposedFilterString + "*", Qt::CaseInsensitive, QRegExp::Wildcard);

		if (m_bSoundExEnabled) {
			QString strSoundEx = m_pSearchStringListModel->soundEx(strDecomposedFilterString, false);

#ifdef SEARCH_COMPLETER_DEBUG_OUTPUT
			qDebug("SoundEx: \"%s\" => %s", m_strFilterFixedString.toUtf8().data(), strSoundEx.toUtf8().data());
#endif

			int nFirstWord = lstBasicWords.indexOf_decomposedWord(expPrefix);
			int nLastWord = ((nFirstWord != -1) ? lstBasicWords.lastIndexOf_decomposedWord(expPrefix) : -1);
			int nNumWords = ((nFirstWord != -1) ? (nLastWord - nFirstWord + 1) : 0);

#ifdef SEARCH_COMPLETER_DEBUG_OUTPUT
			qDebug("Prefix: \"%s\"  expPrefix: \"%s\"  nFirst: %d  nLast: %d", strDecomposedFilterString.toUtf8().data(), expPrefix.pattern().toUtf8().data(), nFirstWord, nLastWord);
#endif

			const QList<int> &mapSoundEx = m_mapSoundEx[strSoundEx];

			m_nFirstDecomposedMatchStringIndex = nFirstWord;		// Temporarily set first word index to our decomposed list index.  After sorting, we'll find it's new location and change it
			m_nFirstComposedMatchStringIndex = lstBasicWords.indexOf_renderedWord(m_strFilterFixedString, ((m_nFirstDecomposedMatchStringIndex != -1) ? m_nFirstDecomposedMatchStringIndex : 0));

			QList<int> lstMatches;
			lstMatches.reserve(mapSoundEx.size() + nNumWords);
			lstMatches.append(m_mapSoundEx[strSoundEx]);
			while (nFirstWord <= nLastWord) {
				lstMatches.append(nFirstWord);
				++nFirstWord;
			}

			std::sort(lstMatches.begin(), lstMatches.end());

			// Remove Duplicates:
			m_lstMatchedIndexes.reserve(lstMatches.size());
			int nLastValue = -1;
			for (int nRow = 0; nRow < lstMatches.size(); ++nRow) {
				if (lstMatches.at(nRow) != nLastValue) m_lstMatchedIndexes.append(lstMatches.at(nRow));
				nLastValue = lstMatches.at(nRow);
			}

			// Find our translated first word RegExp match:
			bool bFoundComposed = (m_nFirstComposedMatchStringIndex == -1);
			bool bFoundDecomposed = (m_nFirstDecomposedMatchStringIndex == -1);
			if ((!bFoundComposed) || (!bFoundDecomposed)) {
				for (int nRow = 0; nRow < m_lstMatchedIndexes.size(); ++nRow) {
					if (!bFoundComposed) {
						if (m_lstMatchedIndexes.at(nRow) == m_nFirstComposedMatchStringIndex) {
							m_nFirstComposedMatchStringIndex = nRow;
							bFoundComposed = true;
						}
					}
					if (!bFoundDecomposed) {
						if (m_lstMatchedIndexes.at(nRow) == m_nFirstDecomposedMatchStringIndex) {
							m_nFirstDecomposedMatchStringIndex = nRow;
							bFoundDecomposed = true;
						}
					}
					if (bFoundComposed && bFoundDecomposed) break;
				}
			}
		} else {
			m_nFirstDecomposedMatchStringIndex = lstBasicWords.indexOf_decomposedWord(expPrefix);
			m_nFirstComposedMatchStringIndex = lstBasicWords.indexOf_renderedWord(m_strFilterFixedString, ((m_nFirstDecomposedMatchStringIndex != -1) ? m_nFirstDecomposedMatchStringIndex : 0));

#ifdef SEARCH_COMPLETER_DEBUG_OUTPUT
			qDebug("Prefix: \"%s\"  expPrefix: \"%s\"  nFirstMatch: %d", strDecomposedFilterString.toUtf8().data(), expPrefix.pattern().toUtf8().data(), m_nFirstDecomposedMatchStringIndex);
#endif
		}
	}

	if (bResetModel) endResetModel();

	// For some reason, QCompleter doesn't respond to begin/end ModelReset, but does for dataChanged.
	//		Without the following, the model data gets completely out of sync in the QCompleter... Go figure...
	if (bResetModel) emit dataChanged(QModelIndex(), QModelIndex());
}

void CSoundExSearchCompleterFilter::en_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
#ifdef SEARCH_COMPLETER_DEBUG_OUTPUT
	qDebug("SoundExSearchCompleter::en_dataChanged");
#endif

	if ((!topLeft.isValid()) || (!bottomRight.isValid())) {
		emit dataChanged(topLeft, bottomRight);			// If either node is invalid, assume it's some special reset all case, pass-on as-is
		return;
	}
	if (m_lstMatchedIndexes.size() == 0) {				// If our filter is empty, pass on unchanged
		emit dataChanged(topLeft, bottomRight);
		return;
	}

	int nRowFirst = 0;
	int nRowLast = 0;
	for (int nRow = 0; nRow < m_lstMatchedIndexes.size(); ++ nRow) {
		if (m_lstMatchedIndexes.at(nRow) < topLeft.row()) {
			nRowFirst = nRow;
			nRowLast = nRow;
		} else {
			if (m_lstMatchedIndexes.at(nRow) < bottomRight.row()) {
				nRowLast = nRow;
			}
		}
	}
	emit dataChanged(createIndex(nRowFirst, 0), createIndex(nRowLast, 0));
}


// ============================================================================

CSoundExSearchCompleterFilter::SOUNDEX_LANGUAGES_ENUM CSoundExSearchCompleterFilter::languageValue(const QString &strLanguage)
{
	SOUNDEX_LANGUAGES_ENUM nLanguage = SELE_UNKNOWN;
	if (strLanguage.compare("en", Qt::CaseInsensitive) == 0) nLanguage = SELE_ENGLISH;
	if (strLanguage.compare("fr", Qt::CaseInsensitive) == 0) nLanguage = SELE_FRENCH;
	if (strLanguage.compare("es", Qt::CaseInsensitive) == 0) nLanguage = SELE_SPANISH;
	if (strLanguage.compare("de", Qt::CaseInsensitive) == 0) nLanguage = SELE_GERMAN;

	return nLanguage;
}

QString CSoundExSearchCompleterFilter::soundEx(const QString &strWordIn, SOUNDEX_LANGUAGES_ENUM nLanguage, int nLength, SOUNDEX_OPTION_MODE_ENUM nOption)
{
// strWordIn should already be decomposed:
//	QString strSoundEx = CSearchStringListModel::decompose(strWordIn, true).toUpper();
	QString strSoundEx = strWordIn.toUpper();
	int nSoundExLen = 0;

	if (nLanguage == SELE_UNKNOWN) nLanguage = SELE_ENGLISH;		// Use default functionality for English if the language isn't defined

	if (nLanguage != SELE_ENGLISH) nOption = SEOME_CLASSIC;			// Currently only support enhanced and census modes in English

	if ((nOption == SEOME_CENSUS_NORMAL) || (nOption == SEOME_CENSUS_SPECIAL)) nLength = 4;
	if (nLength) nSoundExLen = nLength;
	if (nSoundExLen > 10) nSoundExLen = 10;
	if (nSoundExLen < 4) nSoundExLen = 4;

	for (int i = 0; i < strSoundEx.size(); ++i) {
		if (!strSoundEx.at(i).isLetter()) strSoundEx[i] = QChar(' ');
	}
	strSoundEx = strSoundEx.trimmed();

	if (strSoundEx.isEmpty()) return QString();

	// Enhanced Non-Census Mode:
	if (nOption == SEOME_ENHANCED) {
		strSoundEx.remove(QRegExp("^P(?=[SF])"));				// Replace PS at start of word with S and PF at start of word with F
		strSoundEx.replace(QRegExp("^[AI](?=[AEIO])"), "E");	// Replace A or I with E at start of word when followed by [AEIO]

		strSoundEx.replace(QRegExp("DG"), "G");					// Replace DG with G
		strSoundEx.replace(QRegExp("GH"), "H");					// Replace GH with H
		strSoundEx.replace(QRegExp("[KG]N"), "N");				// Replace KN and GN (not "ng") with N
		strSoundEx.replace(QRegExp("MB"), "M");					// Replace MB with M
		strSoundEx.replace(QRegExp("PH"), "F");					// Replace PH wtih F
		strSoundEx.replace(QRegExp("TCH"), "CH");				// Replace TCH with CH
		strSoundEx.replace(QRegExp("MP(?=[STZ])"), "M");		// Replace MP with M when followed by S, Z, or T
	}

	assert(strSoundEx.size());

	// The following must be done AFTER the multi-letter
	//		replacements above since these could change
	//		the first character:
	QChar chrFirstLetter = strSoundEx[0];						// Note: We already know we have at least one character

	strSoundEx = strSoundEx.mid(1);

	// For "Normal" Census SoundEx, remove H and W
	//		before performing the test for adjacent
	//		digits:
	if ((nOption == SEOME_CLASSIC) || (nOption == SEOME_ENHANCED)) {
		strSoundEx.remove(QRegExp("[HW]"));						// Note, strSoundEx[0] won't get removed here because of above preserving it
	}

	// Perform classic SoundEx replacements:
	switch (nLanguage) {
		case SELE_ENGLISH:
//			https://en.wikipedia.org/wiki/Soundex
//			https://creativyst.com/Doc/Articles/SoundEx1/SoundEx1.htm
//
//			The correct value can be found as follows:
//
//			1. Retain the first letter of the name and drop all other occurrences of a, e, i, o, u, y, h, w.
//			2. Replace consonants with digits as follows (after the first letter):
//					b, f, p, v => 1
//					c, g, j, k, q, s, x, z => 2
//					d, t => 3
//					l => 4
//					m, n => 5
//					r => 6
//			3. If two or more letters with the same number are adjacent in the original name (before step 1),
//				only retain the first letter; also two letters with the same number separated by 'h' or 'w' are
//				coded as a single number, whereas such letters separated by a vowel are coded twice. This rule
//				also applies to the first letter.
//			4. Iterate the previous step until you have one letter and three numbers. If you have too few letters
//				in your word that you can't assign three numbers, append with zeros until there are three numbers.
//				If you have more than 3 letters, just retain the first 3 numbers.

			strSoundEx.replace(QRegExp("[AEIOUYHW]"), "0");				// [AEIOUYHW] => 0 (Special case for doing separation, except H or W as found above)
			strSoundEx.replace(QRegExp("[BPFV]"), "1");					// [BPFV] => 1
			strSoundEx.replace(QRegExp("[CSGJKQXZ]"), "2");				// [CSGJKQXZ] => 2
			strSoundEx.replace(QRegExp("[DT]"), "3");					// [DT] => 3
			strSoundEx.replace(QChar('L'), QChar('4'));					// L => 4
			strSoundEx.replace(QRegExp("[MN]"), "5");					// [MN] => 5
			strSoundEx.replace(QChar('R'), QChar('6'));					// R => 6
			break;

		case SELE_FRENCH:
//			https://fr.wikipedia.org/wiki/Soundex
//
//			L'algorithme exact procède comme suit :
//
//			    1. Supprimer les éventuels 'espace' initiaux
//			    2. Mettre le mot en majuscule
//			    3. Garder la première lettre
//			    4. Conserver la première lettre de la chaîne
//			    5. Supprimer toutes les occurrences des lettres : a, e, h, i, o, u, w, y (à moins
//					que ce ne soit la première lettre du nom)
//			    6. Attribuer une valeur numérique aux lettres restantes de la manière suivante :
//			        Version pour le français :
//			            1 = B, P
//			            2 = C, K, Q
//			            3 = D, T
//			            4 = L
//			            5 = M, N
//			            6 = R
//			            7 = G, J
//			            8 = X, Z, S
//			            9 = F, V
//			    7. Si deux lettres (ou plus) avec le même nombre sont adjacentes dans le nom
//					d'origine, ou s'il n'y a qu'un h ou un w entre elles, alors on ne retient
//					que la première de ces lettres.
//			    8. Renvoyer les quatre premiers octets complétés par des zéros.

			strSoundEx.replace(QRegExp("[AEIOUYHW]"), "0");				// [AEIOUYHW] => 0 (Special case for doing separation, except H or W as found above)
			strSoundEx.replace(QRegExp("[BP]"), "1");					// [BP] => 1
			strSoundEx.replace(QRegExp("[CKQ]"), "2");					// [CKQ] => 2
			strSoundEx.replace(QRegExp("[DT]"), "3");					// [DT] => 3
			strSoundEx.replace(QChar('L'), QChar('4'));					// L => 4
			strSoundEx.replace(QRegExp("[MN]"), "5");					// [MN] => 5
			strSoundEx.replace(QChar('R'), QChar('6'));					// R => 6
			strSoundEx.replace(QRegExp("[GJ]"), "7");					// [GJ] => 7
			strSoundEx.replace(QRegExp("[XZS]"), "8");					// [XZS] => 8
			strSoundEx.replace(QRegExp("[FV]"), "9");					// [FV] => 9
			break;

		case SELE_SPANISH:
//			http://oraclenotepad.blogspot.com/2008/03/soundex-en-espaol.html
//
//			Los pasos básicos son:
//
//			    1. Retener la primera letra de la cadena. Tener en cuenta las letras dobles como CH y LL.
//			    2. Remover todas las ocurrencias de las letras siguientes a partir de la segunda posición: a, e, i, o, u, h, w, y (cuando suena como vocal i )
//			    3. Asignar números a las siguientes letras (luego de la primera):
//			        b, f, p, v = 1
//			        c, g, j, k, q, s, x, z = 2
//			        d, t = 3
//			        l = 4
//			        m, n = 5
//			        r = 6
//			        ll, y, ch = 7
//			    4. Si hay números consecutivos, dejar solamente uno en la serie.
//			    5. Retornar los cuatro primeros caracteres, si son menos de cuatro completar con ceros.
//
//			SOUNDESP es un proyecto abierto y es bienvenido cualquier comentario para mejorar su implementación.
//
//			Y Rules:
//			--------
//			(Keep) : Y -> when alone, or after a vowel, or followed by a consonant, or at the end of a word,
//					is a vowel, and sounds as e or ee in English: Hoy y mañana (today and tomorrow), o’-e ee mah-nyah’-nah
//
//			(remove) : Y -> before a vowel in the same syllable, or between two vowels in the same word, is a consonant,
//					and sounds like the English y in the words yard, yell, you

			strSoundEx.replace("CH", "7");								// Proceso letras dobles primero.
			strSoundEx.replace("LL", "7");
			strSoundEx.replace(QRegExp("Y$"), "7");						// Y al final de la palabra.
			strSoundEx.replace(QRegExp("Y(?=[bcdfghjklmnpqrstvwxz])"), "7");	// Y antes de una consonante.
			strSoundEx.replace(QRegExp("[AEIOU]Y"), "07");				// Y después de una vocal : Note: QRegExp doesn't support look-behind, so combining this with "0" substitution for vowels that follows

			strSoundEx.replace(QRegExp("[AEIOUYHW]"), "0");				// [AEIOUYHW] => 0 (Special case for doing separation, except H or W as found above)
			strSoundEx.replace(QRegExp("[BPFV]"), "1");					// [BPFV] => 1
			strSoundEx.replace(QRegExp("[CSGJKQXZ]"), "2");				// [CSGJKQXZ] => 2
			strSoundEx.replace(QRegExp("[DT]"), "3");					// [DT] => 3
			strSoundEx.replace(QChar('L'), QChar('4'));					// L => 4
			strSoundEx.replace(QRegExp("[MN]"), "5");					// [MN] => 5
			strSoundEx.replace(QChar('R'), QChar('6'));					// R => 6
			break;

		case SELE_GERMAN:
//			http://www.sound-ex.de/soundex_verfahren.htm
//			https://de.wikipedia.org/wiki/Soundex
//
//			Grundregeln
//
//			Jeder Soundex-Code besteht aus einem Buchstaben gefolgt von drei Ziffern, z.B.
//			W-213 für Wikipedia. Hat das zu codierende Wort soviele Buchstaben, daß man mehr
//			Ziffern erzeugen könnte, bricht man nach der dritten Ziffer ab. Hat das Wort zu
//			wenige Buchstaben, füllt man die letzten Ziffern mit 0-en auf. Der asiatische Name
//			Lee wird also als L-000 codiert.
//			Ziffer => Repräsentatierte Buchstaben
//			1 => B, F, P, V
//			2 => C, G, J, K, Q, S, X, Z
//			3 => D, T
//			4 => L
//			5 => M, N
//			6 => R
//
//			Die Vokale A, E, I, O und U, als auch die Konsonanten H, W und Y sind zu ignorieren,
//			allerdings nicht an erster Stelle als führender Buchstabe. Erweiternd für die deutsche
//			Sprache definiert man: Die Umlaute Ä, Ö und Ü sind zu ignorieren, das "scharfe S" ß wird
//			wie das einfache S als 2 codiert.
//
//			Doppelte Buchstaben
//
//			Doppelte Buchstaben, wie in Kallbach sind wie ein einzelner Buchstabe zu betrachten.
//
//			• Kallbach wird daher zu K-412 (K -> K, A wird verworfen, L -> 4, 2. L wird verworfen,
//				B -> 1, 2. A wird verworfen, C -> 2, Abbruch weil wir bereits 3 Ziffern haben).
//
//			Aufeinanderfolgende Buchstaben mit gleichem Soundex-Code
//
//			Werden wie gleiche Buchstaben behandelt.
//
//			• Hackelmeier wird daher zu H-245 (H -> H, A wird verworfen, C -> 2, K wird verworfen weil
//				auch = 2, E wird verworfen, L -> 4, M -> 5, Abbruch weil wir bereits 3 Ziffern haben.
//
//			Namenszusätze
//
//			Namenszusätze können ignoriert werden, oder normal mitkodiert werden. Bei der Suche ist
//			dies entsprechend zu berücksichtigen, d.h. es sind ggf. zwei Suchen durchzuführen.
//
//			• von Neumann wird einmal zu V-555 oder zu N-550 (beachte auch die folgende Regel)
//
//			Konsonantentrennung
//
//			Werden zwei Konsonanten mit dem gleichen Soundex-Code durch ein Vokal (oder Y) getrennt,
//			so wird der rechte Konsonant NICHT verworfen. Ist allerdings ein H oder ein W das Trennzeichen,
//			so wird der rechte Konsonant wie bei der Aufeinanderfolgende Buchstaben-Regel verworfen.

			strSoundEx.replace(QRegExp("[AEIOUYHW]"), "0");				// [AEIOUYHW] => 0 (Special case for doing separation, except H or W as found above)
			strSoundEx.replace(QRegExp("[BPFV]"), "1");					// [BPFV] => 1
			strSoundEx.replace(QRegExp("[CSGJKQXZ]"), "2");				// [CSGJKQXZ] => 2
			strSoundEx.replace(QRegExp("[DT]"), "3");					// [DT] => 3
			strSoundEx.replace(QChar('L'), QChar('4'));					// L => 4
			strSoundEx.replace(QRegExp("[MN]"), "5");					// [MN] => 5
			strSoundEx.replace(QChar('R'), QChar('6'));					// R => 6

			// TODO : Complete German Rules
			break;

		default:
			break;
	}

	// Remove extra equal adjacent digits:
	for (int i = 0; i < (strSoundEx.size()-1); /* Increment inside loop */) {
		if (strSoundEx[i] == strSoundEx[i+1]) {
			strSoundEx.remove(i+1, 1);
		} else {
			++i;
		}
	}

	// Remove spaces and 0's
	strSoundEx.remove(QChar(' '));
	strSoundEx.remove(QChar('0'));

	// Replace first Letter and Right-pad with zeros:
	QString strZeros;
	strZeros.fill(QChar('0'), nSoundExLen);
	strSoundEx = chrFirstLetter + strSoundEx + strZeros;
	strSoundEx = strSoundEx.left(nSoundExLen);

	return strSoundEx;
}

// ============================================================================

#if QT_VERSION < 0x050000

bool CComposingCompleter::eventFilter(QObject *obj, QEvent *ev)
{
	// The act of popping our completer, will cause the inputContext to
	//		shift focus from the editor to the popup and after dismissing the
	//		popup, it doesn't go back to the editor.  So, since we are eating
	//		FocusOut events in the popup, push the inputContext focus back to
	//		the editor when we "focus out".  It's our focusProxy anyway:
	if ((ev->type() == QEvent::FocusOut) && (obj == widget())) {
		if ((popup()) && (popup()->isVisible())) {
			QInputContext *pInputContext = popup()->inputContext();
			if (pInputContext) pInputContext->setFocusWidget(popup());
		}
	}

	return QCompleter::eventFilter(obj, ev);
}

#endif

// ============================================================================

#ifdef QT_WIDGETS_LIB

CStrongsDictionarySearchCompleter::CStrongsDictionarySearchCompleter(CDictionaryDatabasePtr pDictionary, const QTextEdit &editorWord, QWidget *parentWidget)
	:	SearchCompleter_t(parentWidget),
		m_pStrongsListModel(nullptr)
{
	m_pStrongsListModel = new CSearchStrongsDictionaryListModel(pDictionary, editorWord, this);
	setModel(m_pStrongsListModel);
#if QT_VERSION >= 0x050200		// Filter Mode was introduced in Qt 5.2
	setFilterMode(Qt::MatchStartsWith);
#endif
}

void CStrongsDictionarySearchCompleter::selectFirstMatchString()
{
}

void CStrongsDictionarySearchCompleter::setFilterMatchString()
{
	assert(m_pStrongsListModel != nullptr);
	m_strFilterMatchString = m_pStrongsListModel->cursorWord();
	setCompletionPrefix(m_strFilterMatchString);
}

void CStrongsDictionarySearchCompleter::setWordsFromPhrase(bool bForceUpdate)
{
	Q_UNUSED(bForceUpdate);
}

#endif

// ============================================================================
