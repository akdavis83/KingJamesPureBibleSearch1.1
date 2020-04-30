/****************************************************************************
**
** Copyright (C) 2012 Donna Whisnant, a.k.a. Dewtronics.
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

#ifndef DBSTRUCT_H
#define DBSTRUCT_H

#include "dbDescriptors.h"

#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <numeric>
#include <stdint.h>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QList>
#include <QMap>
#include <QMultiMap>
#include <QVariant>
#include <QPair>
#include <QMetaType>
#include <QDataStream>
#include <QSharedPointer>
#include <QObject>
#ifndef NOT_USING_SQL
#include <QSqlDatabase>
#endif

#include <assert.h>

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof(x[0]))
#endif

// ============================================================================

#define KJPBS_CCDB_VERSION		1			// Current version of our CCDB file format

// ============================================================================

// Forward declarations:
class CBibleDatabase;
class CBasicHighlighter;

// ============================================================================

// CRelIndex Masks:
#define RIMASK_HEADING	0x10
#define RIMASK_BOOK		0x08
#define RIMASK_CHAPTER	0x04
#define RIMASK_VERSE	0x02
#define RIMASK_WORD		0x01
#define RIMASK_ALL		0x1F

class CRelIndex {
public:
	CRelIndex(const CRelIndex &ndx)
		:	m_ndx(ndx.index())
	{
	}
	CRelIndex(uint32_t ndx = 0)
		:	m_ndx(ndx)
	{
	}
	CRelIndex(const QString &strAnchor)
		:	m_ndx(strAnchor.toUInt())
	{
	}
	CRelIndex(uint32_t nBk, uint32_t nChp, uint32_t nVrs, uint32_t nWrd)
	{
		setIndex(nBk, nChp, nVrs, nWrd);
	}
	~CRelIndex() { }

	inline bool isColophon() const {
		return ((book() != 0) && (chapter() == 0) && (verse() == 0) && (word() != 0));
	}
	inline bool isSuperscription() const {
		return ((book() != 0) && (chapter() != 0) && (verse() == 0) && (word() != 0));
	}

	inline QString asAnchor() const {			// Anchor is a text string unique to this reference
		return QString("%1").arg(m_ndx);
	}

	static uint32_t maxBookCount() { return 0xFF; }
	inline uint32_t book() const { return ((m_ndx >> 24) & 0xFF); }
	inline void setBook(uint32_t nBk) {
		m_ndx = ((m_ndx & 0x00FFFFFF) | ((nBk & 0xFF) << 24));
	}
	static uint32_t maxChapterCount() { return 0xFF; }
	inline uint32_t chapter() const { return ((m_ndx >> 16) & 0xFF); }
	inline void setChapter(uint32_t nChp) {
		m_ndx = ((m_ndx & 0xFF00FFFF) | ((nChp & 0xFF) << 16));
	}
	static uint32_t maxVerseCount() { return 0xFF; }
	inline uint32_t verse() const { return ((m_ndx >> 8) & 0xFF); }
	inline void setVerse(uint32_t nVrs) {
		m_ndx = ((m_ndx & 0xFFFF00FF) | ((nVrs & 0xFF) << 8));
	}
	static uint32_t maxWordCount() { return 0xFF; }
	inline uint32_t word() const { return (m_ndx & 0xFF); }
	inline void setWord(uint32_t nWrd) {
		m_ndx = ((m_ndx & 0xFFFFFF00) | (nWrd & 0xFF));
	}
	inline bool isSet() const { return (m_ndx != 0); }
	inline void clear() { m_ndx = 0; }

	inline uint32_t index() const { return m_ndx; }
	inline void setIndex(uint32_t nBk, uint32_t nChp, uint32_t nVrs, uint32_t nWrd) {
		m_ndx = (((nBk & 0xFF) << 24) | ((nChp & 0xFF) << 16) | ((nVrs & 0xFF) << 8) | (nWrd & 0xFF));
	}
	inline void setIndex(uint32_t ndx) {
		m_ndx = ndx;
	}

	inline bool operator<(const CRelIndex &ndx) const {
		return (index() < ndx.index());
	}
	inline bool operator<=(const CRelIndex &ndx) const {
		return (index() <= ndx.index());
	}
	inline bool operator>(const CRelIndex &ndx) const {
		return (index() > ndx.index());
	}
	inline bool operator>=(const CRelIndex &ndx) const {
		return (index() >= ndx.index());
	}
	inline bool operator==(const CRelIndex &ndx) const {
		return (index() == ndx.index());
	}
	inline bool operator!=(const CRelIndex &ndx) const {
		return (index() != ndx.index());
	}

	static CRelIndex navigationIndexFromLogicalIndex(const CRelIndex &ndxLogical)
	{
		CRelIndex ndxVerse = ndxLogical;

		if (ndxVerse.isSet()) {
			if (((ndxVerse.chapter() == 0) || (ndxVerse.verse() == 0)) &&
				(ndxVerse.word() == 0)) {
				if (ndxVerse.chapter() == 0) ndxVerse.setChapter(1);
				ndxVerse.setVerse(1);
				ndxVerse.setWord(1);
			}
		}

		return ndxVerse;
	}

private:
	uint32_t m_ndx;
};
inline QDataStream& operator<<(QDataStream &out, const CRelIndex &ndx) {
	out << ndx.index();
	return out;
}
inline QDataStream& operator>>(QDataStream &in, CRelIndex &ndx) {
	uint32_t anIndex;
	in >> anIndex;
	ndx.setIndex(anIndex);
	return in;
}
Q_DECLARE_METATYPE(CRelIndex)
Q_DECLARE_METATYPE(uint32_t)

// ============================================================================

#ifdef USE_EXTENDED_INDEXES
class CRelIndexEx : public CRelIndex {
public:
	CRelIndexEx(const CRelIndexEx &ndx)
		:	CRelIndex(ndx.index()),
			m_ndxEx(ndx.m_ndxEx)
	{
	}
	CRelIndexEx(uint64_t ndxEx = 0)
	{
		setIndexEx(ndxEx);
	}
	CRelIndexEx(const QString &strAnchor)
		:	CRelIndex(strAnchor),
			m_ndxEx(1)
	{
	}
	CRelIndexEx(uint32_t nBk, uint32_t nChp, uint32_t nVrs, uint32_t nWrd, uint32_t nLtr)
	{
		setIndexEx(nBk, nChp, nVrs, nWrd, nLtr);
	}
	~CRelIndexEx() { }

	static uint32_t maxLetterCount() { return 0xFFFFFFFF; }
	inline uint32_t letter() const { return m_ndxEx; }
	inline void setLetter(uint32_t nLtr) {
		m_ndxEx = nLtr;
	}

	inline bool isSet() const { return ((CRelIndex::isSet()) || (m_ndxEx != 0)); }
	inline void clear() { CRelIndex::clear(); m_ndxEx =0; }

	inline uint64_t indexEx() const
	{
		return ((static_cast<uint64_t>(index()) << 32) || static_cast<uint64_t>(m_ndxEx));
	}
	inline void setIndexEx(uint32_t nBk, uint32_t nChp, uint32_t nVrs, uint32_t nWrd, uint32_t nLtr)
	{
		setIndex(nBk, nChp, nVrs, nWrd);
		m_ndxEx = nLtr;
	}
	inline void setIndexEx(uint64_t ndx)
	{
		setIndex(static_cast<uint32_t>(ndx >> 32));
		m_ndxEx = static_cast<uint32_t>(ndx & 0xFFFFFFFF);
	}

private:
	uint32_t m_ndxEx;				// Extended portion of index
};
#endif

// ============================================================================

// Pair representing X (first) of Y (second) things:
class TCountOf : public QPair<unsigned int, unsigned int>
{
public:
	explicit inline TCountOf(unsigned int x = 0, unsigned int y = 0)
		:	QPair<unsigned int, unsigned int>(x, y) { }
};

class CBibleDatabase;		// Forward declaration:

class CRefCountCalc			// Calculates the reference count information for creating ToolTips and indices
{
public:
	enum REF_TYPE_ENUM {
		RTE_TESTAMENT = 0,
		RTE_BOOK = 1,
		RTE_CHAPTER = 2,
		RTE_VERSE = 3,
		RTE_WORD = 4
	};

	CRefCountCalc(const CBibleDatabase *pBibleDatabase, REF_TYPE_ENUM nRefType, const CRelIndex &refIndex);
	~CRefCountCalc() { }

	REF_TYPE_ENUM refType() const { return m_nRefType; }
	CRelIndex refIndex() const { return m_ndxRef; }

	TCountOf ofBible() const { return m_nOfBible; }
	TCountOf ofTestament() const { return m_nOfTst; }
	TCountOf ofBook() const { return m_nOfBk; }
	TCountOf ofChapter() const { return m_nOfChp; }
	TCountOf ofVerse() const { return m_nOfVrs; }

private:
	CRelIndex m_ndxRef;			// Relative Index

	REF_TYPE_ENUM m_nRefType;	// Type of Reference these counts are for

	// All entries above the type specified will be valid:
	TCountOf m_nOfBible;	// Testament, Book, Chapter, Verse, Word of the whole Bible
	TCountOf m_nOfTst;		// Book, Chapter, Verse, Word of the Testament
	TCountOf m_nOfBk;		// Chapter, Verse, Word of the Book
	TCountOf m_nOfChp;		// Verse, Word of the Chapter
	TCountOf m_nOfVrs;		// Word of the Verse
};


// ============================================================================

struct RelativeIndexSortPredicate {
	bool operator() (const CRelIndex &v1, const CRelIndex &v2) const
	{
		return (v1.index() < v2.index());
	}
};

struct XformLower {
	int operator()(int c)
	{
		return tolower(c);
	}
};

typedef std::vector<uint32_t> TNormalizedIndexList;			// Normalized Index List for words into book/chapter/verse/word
typedef std::vector<CRelIndex> TRelativeIndexList;			// Relative Index List for words into book/chapter/verse/word
typedef std::set<CRelIndex, RelativeIndexSortPredicate> TRelativeIndexSet;		// Relative Index Set for words into book/chapter/verse/word
class TCrossReferenceMap : public std::map<CRelIndex, TRelativeIndexSet, RelativeIndexSortPredicate>		// Map of Relative Index to Relative Index Set, used for cross-references (such as User Notes Database cross-reference, etc)
{
public:
	TCrossReferenceMap()
		:	std::map<CRelIndex, TRelativeIndexSet, RelativeIndexSortPredicate>(),
			m_bNoWordRefs(true)
	{

	}

	TCrossReferenceMap(const TCrossReferenceMap &aMap)
		:	std::map<CRelIndex, TRelativeIndexSet, RelativeIndexSortPredicate>(aMap),
			m_bNoWordRefs(true)
	{

	}

	inline bool haveCrossReferencesFor(const CRelIndex &ndx) const
	{
		return (find(relIndexMaskWord(ndx)) != end());
	}
	inline bool haveCrossReference(const CRelIndex &ndxFirst, const CRelIndex &ndxSecond) const
	{
		const TRelativeIndexSet refs = crossReferencesFor(ndxFirst);
		TRelativeIndexSet::const_iterator itr = refs.find(relIndexMaskWord(ndxSecond));
		return (itr != refs.end());
	}
	inline const TRelativeIndexSet crossReferencesFor(const CRelIndex &ndx) const
	{
		TCrossReferenceMap::const_iterator itr = find(relIndexMaskWord(ndx));
		if (itr == end()) return TRelativeIndexSet();
		return (itr->second);
	}
	TCrossReferenceMap createScopedMap(const CBibleDatabase *pBibleDatabase) const;

private:
	// TODO : If we ever change this to allow WordRefs, we must update calls in
	//		VerseListModel to properly mask the word for the special "word 1"
	//		indexes used for verses to distinguish other types
	inline CRelIndex relIndexMaskWord(const CRelIndex &ndx) const
	{
		if (m_bNoWordRefs) {
			return CRelIndex(ndx.book(), ndx.chapter(), ndx.verse(), 0);
		}
		return ndx;
	}

private:
	bool m_bNoWordRefs;
};

// ============================================================================

// Testaments -- Table of Testaments:
//
class CTestamentEntry
{
public:
	CTestamentEntry(const QString &strTstName = QString())
	:	m_strTstName(strTstName),
		m_nNumBk(0),
		m_nNumChp(0),
		m_nNumVrs(0),
		m_nNumWrd(0)
#ifdef USE_EXTENDED_INDEXES
		,m_nNumLtr(0)
#endif
	{ }
	~CTestamentEntry() { }

	QString  m_strTstName;		// Name of testament (display name)
	unsigned int m_nNumBk;		// Number of Books in this testament
	unsigned int m_nNumChp;		// Number of Chapters in this testament
	unsigned int m_nNumVrs;		// Number of Verses in this testament
	unsigned int m_nNumWrd;		// Number of Words in this testament
#ifdef USE_EXTENDED_INDEXES
	uint32_t m_nNumLtr;		// Number of Letters in this testament
#endif
};

typedef std::vector<CTestamentEntry> TTestamentList;		// Index by nTst-1

// Bible -- Bible Entry (Derived from CTestamentEntry to keep stats for the whole Bible)
class CBibleEntry : public CTestamentEntry
{
public:
	CBibleEntry()
	:	CTestamentEntry(QObject::tr("Entire Bible", "Scope")),
		m_nNumTst(0)
	{ }
	~CBibleEntry() { }

	unsigned int m_nNumTst;		// Number of Testaments
};

// ============================================================================

// Book Categories (i.e. Law, Prophets, Gospels, etc)
//
class CBookCategoryEntry
{
public:
	CBookCategoryEntry(const QString &strCategoryName = QString())
		:	m_strCategoryName(strCategoryName)
	{ }
	~CBookCategoryEntry() { }

	QString m_strCategoryName;				// Category Name
	std::set<uint32_t> m_setBooksNum;		// Set of Book Numbers in this Set
};

typedef std::vector<CBookCategoryEntry> TBookCategoryList;		// Index by nCat-1

// ============================================================================

// Books -- (Table of Contents):
//
class CBookEntry
{
public:
	CBookEntry()
	:   m_nTstBkNdx(0),
		m_nTstNdx(0),
		m_nNumChp(0),
		m_nNumVrs(0),
		m_nNumWrd(0),
		m_nWrdAccum(0),
#ifdef USE_EXTENDED_INDEXES
		m_nNumLtr(0),
		m_nLtrAccum(0),
#endif
		m_bHaveColophon(false)
	{ }
	~CBookEntry() { }

	uint32_t m_nTstBkNdx;		// Testament Book Index (Index within the books of the testament) 1-39 or 1-27
	uint32_t m_nTstNdx;			// Testament Index (1=Old, 2=New, etc)
	uint32_t m_nCatNdx;			// Category Index
	QString m_strBkName;		// Name of book (display name)
	QStringList m_lstBkAbbr;	// Book Abbreviations (Always at LEAST two entries.  First entry is OSIS Abbreviation.  Second entry is Common Abbreviation used in all Abbreviated Book mode rendering.  Others are common abbreviations used for matching purposes)
	QString m_strTblName;		// Name of Table for this book
	unsigned int m_nNumChp;		// Number of chapters in this book
	unsigned int m_nNumVrs;		// Number of verses in this book
	unsigned int m_nNumWrd;		// Number of words in this book
	unsigned int m_nWrdAccum;	// Number of accumulated words prior to, but not including this book
#ifdef USE_EXTENDED_INDEXES
	uint32_t m_nNumLtr;			// Number of letters in this book
	uint32_t m_nLtrAccum;		// Number of accumulated letters prior to, but not including this book
#endif
	QString m_strDesc;			// Description (subtitle)

	bool m_bHaveColophon;		// True if this book has a Colophon pseudo-verse (will be indexed as [nBk|0|0|0] in the TVerseEntryMap, ie. nChp==0, nVrs==0)
};

typedef std::vector<CBookEntry> TBookList;	// Index by nBk-1

// ============================================================================

// Chapters -- Book/Chapter Layout:
//
class CChapterEntry
{
public:
	CChapterEntry()
	:   m_nNumVrs(0),
		m_nNumWrd(0),
		m_nWrdAccum(0),
#ifdef USE_EXTENDED_INDEXES
		m_nNumLtr(0),
		m_nLtrAccum(0),
#endif
		m_bHaveSuperscription(false)
	{ }
	~CChapterEntry() { }

	unsigned int m_nNumVrs;		// Number of verses in this chapter
	unsigned int m_nNumWrd;		// Number of words in this chapter
	unsigned int m_nWrdAccum;	// Number of accumulated words prior to, but not including this chapter
#ifdef USE_EXTENDED_INDEXES
	uint32_t m_nNumLtr;			// Number of letters in this chapter
	uint32_t m_nLtrAccum;		// Number of accumulated letters prior to, but not including this chapter
#endif

	bool m_bHaveSuperscription;	// True if this chapter has a Superscription pseudo-verse (will be indexed as [nBk|nChp|0|0] in the TVerseEntryMap, ie. nVrs==0)
};

typedef std::map<CRelIndex, CChapterEntry, RelativeIndexSortPredicate> TChapterMap;	// Index by [nBk|nChp|0|0]

// ============================================================================

// Verses -- Chapter/Verse Layout:
//
class CVerseEntry
{
public:
	enum PILCROW_TYPE_ENUM {
		PTE_NONE = 0,
		PTE_MARKER = 1,				// Example: {verse}[osisID=Gen.5.21]{milestone}[marker=¶,type=x-p]{/milestone}
		PTE_MARKER_ADDED = 2,		// Example: {verse}[osisID=Gen.5.3]{milestone}[marker=¶,subType=x-added,type=x-p]{/milestone}
		PTE_EXTRA = 3				// Example: {verse}[osisID=Gen.5.6]{milestone}[type=x-extra-p]{/milestone}
	};

	CVerseEntry()
	:   m_nNumWrd(0),
		m_nWrdAccum(0),
#ifdef USE_EXTENDED_INDEXES
		m_nNumLtr(0),
		m_nLtrAccum(0),
#endif
		m_nPilcrow(PTE_NONE)
	{ }
	~CVerseEntry() { }

	unsigned int m_nNumWrd;			// Number of words in this verse
	unsigned int m_nWrdAccum;		// Number of accumulated words prior to, but not including this verse
#ifdef USE_EXTENDED_INDEXES
	uint32_t m_nNumLtr;				// Number of letters in this verse
	uint32_t m_nLtrAccum;			// Number of accumulated letters prior to, but not including this verse
#endif
	PILCROW_TYPE_ENUM m_nPilcrow;	// Start of verse Pilcrow Flag (and Pilcrow type)
	QString m_strTemplate;			// Rich Text Creation Template

#ifdef OSIS_PARSER_BUILD
	QString m_strText;			// Rich text (or plain if Rich unavailable) for the verse (Note: for mobile versions, this element can be removed and fetched from the database if needed)
	QStringList m_lstWords;			// Word List for parse extraction
	QStringList m_lstRichWords;		// Word List as Rich Text for parse extraction
	QStringList m_lstParseStack;	// Parse operation stack (used to parse red-letter tags, added word tags, morphology, concordance references, etc.
#endif

};

typedef std::map<CRelIndex, CVerseEntry, RelativeIndexSortPredicate> TVerseEntryMap;		// Index by [nBk|nChp|nVrs|0]

typedef std::vector<TVerseEntryMap> TBookVerseList;		// Index by nBk-1

#ifdef BIBLE_DATABASE_RICH_TEXT_CACHE
typedef std::map<CRelIndex, QString, RelativeIndexSortPredicate> TVerseCacheMap;			// Index by [nBk|nChp|nVrs|0]
typedef std::map<uint, TVerseCacheMap> TSpecVerseCacheMap;							// Specific TVerseCacheMap -- Index by CVerseTextRichifierTags hash
#endif

// ============================================================================

// Words -- Word List and Mapping
//
class CWordEntry
{
public:
	CWordEntry()
	:	m_bCasePreserve(false),
		m_bIsProperWord(false)
	{ }
	~CWordEntry() { }

	QString m_strWord;			// Word Text
	bool m_bCasePreserve;		// Special Word Case Preserve
	bool m_bIsProperWord;		// Proper Words is set to True if a Word and all its Alternate Word Forms begin with a character in the Letter_Uppercase category (and isn't a special ordinary word, as determined in the KJVDataParse tool)
	QStringList m_lstAltWords;	// List of alternate synonymous words for searching (such as hyphenated and non-hyphenated or capital vs lowercase), these are the exact words of the text
	QStringList m_lstDecomposedAltWords;		// Decomposed Words (used for matching), without hyphens
	QStringList m_lstDecomposedHyphenAltWords;	// Decomposed Words (used for matching), with hyphens
	QStringList m_lstDeApostrAltWords;			// Decomposed Words (used for matching), with apostrophies decomposed without hyphens
	QStringList m_lstDeApostrHyphenAltWords;	// Decomposed Words (used for matching), with apostrophies decomposed with hyphens
	QStringList m_lstRenderedAltWords;			// Alt Words as rendered (hyphen/non-hyphen based on proper/ordinary word rules of Bible Database Setting)
	QList<unsigned int> m_lstAltWordCount;		// Count for each alternate word.  This will be the number of entries for this word in the mapping below
	TNormalizedIndexList m_ndxNormalizedMapping;	// Normalized Indexes Mapping into entire Bible

	struct SortPredicate {
		bool operator() (const QString &s1, const QString &s2) const
		{
			return (s1.compare(s2, Qt::CaseSensitive) < 0);
		}
	};
};

typedef std::map<QString, CWordEntry, CWordEntry::SortPredicate> TWordListMap;		// Indexed by lowercase words from word-list

// ============================================================================

// CBasicWordEntry -- Word Entry Virtual base to handle word/decomposedWord/etc commonly for Bible Concordance and Dictionary
//

class CBasicWordEntry
{
public:
	virtual const QString &word() const = 0;
	virtual const QString &decomposedWord() const = 0;
	virtual const QString &decomposedHyphenWord() const = 0;
	virtual const QString &deApostrWord() const = 0;
	virtual const QString &deApostrHyphenWord() const = 0;
	virtual const QString &renderedWord() const = 0;
};

typedef QList<const CBasicWordEntry *> TBasicWordList;

// ============================================================================

// Concordance -- Mapping of words and their Normalized positions:
//

class CConcordanceEntry : public CBasicWordEntry
{
public:
	CConcordanceEntry(TWordListMap::const_iterator itrEntryWord, int nAltWordIndex, int nIndex = 0);
	virtual ~CConcordanceEntry() { }

	CConcordanceEntry & operator=(const CConcordanceEntry &src)
	{
		m_itrEntryWord = src.m_itrEntryWord;
		m_nAltWordIndex = src.m_nAltWordIndex;
		m_nIndex = src.m_nIndex;
		return *this;
	}

	virtual const QString &word() const { return m_itrEntryWord->second.m_lstAltWords.at(m_nAltWordIndex); }
	virtual const QString &decomposedWord() const { return m_itrEntryWord->second.m_lstDecomposedAltWords.at(m_nAltWordIndex); }
	virtual const QString &decomposedHyphenWord() const { return m_itrEntryWord->second.m_lstDecomposedHyphenAltWords.at(m_nAltWordIndex); }
	virtual const QString &deApostrWord() const { return m_itrEntryWord->second.m_lstDeApostrAltWords.at(m_nAltWordIndex); }
	virtual const QString &deApostrHyphenWord() const { return m_itrEntryWord->second.m_lstDeApostrHyphenAltWords.at(m_nAltWordIndex); }
	virtual const QString &renderedWord() const { return m_itrEntryWord->second.m_lstRenderedAltWords.at(m_nAltWordIndex); }
	inline bool isProperWord() const { return m_itrEntryWord->second.m_bIsProperWord; }
	inline int index() const { return m_nIndex; }

#ifdef USE_EXTENDED_INDEXES
	uint32_t letterCount() const
	{
		const QString &strWord = word();
		uint32_t nCount = 0;
		for (int i=0; i<strWord.size(); ++i) {
			if (strWord.at(i).isLetter()) ++nCount;
		}

		return nCount;
	}
	QChar letter(uint32_t nLtr) const		// one-originated nLtr lookup of letter (to follow pattern of CRelIndex)
	{
		// Note: This function returns the decomposed base-form for the letter,
		//	as generally needed by search/analysis consumers.  However, an
		//	alternate variation for future consideration would be to return
		//	a QString instead containing the full-form entity.  Currently,
		//	this is only used in BibleDNA:
		QString strWord = word().normalized(QString::NormalizationForm_D);
		if (nLtr == 0) return QChar();
		--nLtr;					// one-originated
		for (int i=0; i<strWord.size(); ++i) {
			if (strWord.at(i).isLetter()) {
				if (nLtr == 0) return strWord.at(i);
				--nLtr;
			}
		}
		return QChar();
	}
#endif

	inline bool operator==(const CConcordanceEntry &src) const
	{
		return (word().compare(src.word()) == 0);
	}
	inline bool operator!=(const CConcordanceEntry &src) const
	{
		return (word().compare(src.word()) != 0);
	}

private:
	TWordListMap::const_iterator m_itrEntryWord;	// Bible Word Entry from which this was derived (used to lookup details)
	int m_nAltWordIndex;					// Index of Composed Word in the reference CWordEntry (as in the actual text)
	int m_nIndex;							// Index used when sorting and keeping external reference intact
};

typedef QList<CConcordanceEntry> TConcordanceList;
typedef std::map<QString, QString> TSoundExMap;			// Mapping of Composed Word to SoundEx equivalent, used to minimize calculations

struct TConcordanceListSortPredicate {
	static bool ascendingLessThanWordCaseInsensitive(const CConcordanceEntry &s1, const CConcordanceEntry &s2)
	{
		int nDecompCompare = s1.decomposedWord().compare(s2.decomposedWord(), Qt::CaseInsensitive);
		if (nDecompCompare == 0) {
			return (s1.word().compare(s2.word(), Qt::CaseSensitive) < 0);				// Localized case-sensitive within overall case-insensitive
		}
		return (nDecompCompare < 0);
	}

	static bool ascendingLessThanWordCaseSensitive(const CConcordanceEntry &s1, const CConcordanceEntry &s2)
	{
		int nDecompCompare = s1.decomposedWord().compare(s2.decomposedWord(), Qt::CaseSensitive);
		if (nDecompCompare == 0) {
			return (s1.word().compare(s2.word(), Qt::CaseSensitive) < 0);
		}
		return (nDecompCompare < 0);
	}
};

// ============================================================================

// Footnotes -- Footnote List and Mapping
//		Note: This works consistently for book-only footnotes or colophons,
//		chapter footnotes or superscriptions, verse footnotes, and even word
//		footnotes (or translation lemmas) if we wish.  The index into the
//		map is the complete CRelIndex style.  For book-only, for example, the
//		chapter, verse, and word will be 0.  For a chapter note, the verse and
//		word will be 0, etc.  This allows us to easily and quickly query for the
//		type of note we need.
//

class CFootnoteEntry
{
public:
	CFootnoteEntry()
	{ }
	~CFootnoteEntry() { }

	QString htmlText(const CBibleDatabase *pBibleDatabase = NULL) const;		// Formatted HTML to insert into Scripture Browser (Database is needed only if doing footnotes with embedded scripture or cross-refs, etc)
	QString plainText(const CBibleDatabase *pBibleDatabase = NULL) const;		// Formatted PlainText rendering (Database is needed only if doing footnotes with embedded scripture or cross-refs, etc)

	QString text() const		// We'll use a function to fetch the text (on mobile this can be a database lookup if need be)
	{
		return m_strText;
	}
	void setText(const QString &strText)		// This can be a no-op on mobile if doing direct database lookups
	{
		m_strText = strText;
	}

private:
	QString m_strText;			// Rich text (or plain if Rich unavailable) for the footnote (Note: for mobile versions, this element can be removed and fetched from the database if needed)
};

typedef std::map<CRelIndex, CFootnoteEntry, RelativeIndexSortPredicate> TFootnoteEntryMap;		// Index by [nBk|nChp|nVrs|nWrd]

// ============================================================================

// Phrases -- Common and Saved Search Phrase Lists:
//

class CParsedPhrase;		// Forward declaration

class CPhraseEntry
{
public:
	CPhraseEntry(const QString &strEncodedText = QString(), const QVariant &varExtraInfo = QVariant());
	CPhraseEntry(const CParsedPhrase &aPhrase);
	~CPhraseEntry();

	void clear();

	void setFromPhrase(const CParsedPhrase &aPhrase);

	inline const QString &text() const { return m_strPhrase; }
	QString textEncoded() const;
	void setText(const QString &strText);
	void setTextEncoded(const QString &strText);

	inline bool caseSensitive() const { return m_bCaseSensitive; }
	inline void setCaseSensitive(bool bCaseSensitive) { m_bCaseSensitive = bCaseSensitive; }

	inline bool accentSensitive() const { return m_bAccentSensitive; }
	inline void setAccentSensitive(bool bAccentSensitive) { m_bAccentSensitive = bAccentSensitive; }

	inline bool isExcluded() const { return m_bExclude; }
	inline void setExclude(bool bExclude) { m_bExclude = bExclude; }

	inline bool isDisabled() const { return m_bDisabled; }
	inline void setDisabled(bool bDisabled) { m_bDisabled = bDisabled; }

	inline QVariant extraInfo() const { return m_varExtraInfo; }
	inline void setExtraInfo(const QVariant &varExtraInfo) { m_varExtraInfo = varExtraInfo; }

	inline bool operator==(const CPhraseEntry &src) const
	{
		return ((m_bCaseSensitive == src.m_bCaseSensitive) &&
				(m_bAccentSensitive == src.m_bAccentSensitive) &&
				(m_bExclude == src.m_bExclude) &&
				// Don't compare m_bDisabled because that doesn't affect "equality"
				(m_strPhrase.compare(src.m_strPhrase, Qt::CaseSensitive) == 0));
	}
	inline bool operator!=(const CPhraseEntry &src) const
	{
		return (!(operator==(src)));
	}

	bool operator==(const CParsedPhrase &src) const;		// Implemented in PhraseEdit.cpp, where CParsedPhrase is defined
	bool operator!=(const CParsedPhrase &src) const;		// Implemented in PhraseEdit.cpp, where CParsedPhrase is defined

	static const QChar encCharCaseSensitive() { return QChar(0xA7); } 			// Section Sign = Case-Sensitive
	static const QChar encCharAccentSensitive() { return QChar(0xA4); }			// Current Sign = Accent-Sensitive
	static const QChar encCharExclude() { return QChar(0x2209); }				// Not an Element of = Exclude
	static const QChar encCharDisabled() { return QChar(0xAC); }				// Not Sign = Disable flag

	// From Qt 5.13.0 QtPrivate in qhashfunctions.h
	struct QHashCombineCommutative {
		// QHashCombine is a good hash combiner, but is not commutative,
		// ie. it depends on the order of the input elements. That is
		// usually what we want: {0,1,3} should hash differently than
		// {1,3,0}. Except when it isn't (e.g. for QSet and
		// QHash). Therefore, provide a commutative combiner, too.
		typedef uint result_type;
		template <typename T>
		Q_DECL_CONSTEXPR result_type operator()(uint seed, const T &t) const Q_DECL_NOEXCEPT_EXPR(noexcept(qHash(t)))
		{ return seed + qHash(t); } // don't use xor!
	};

private:
	bool m_bCaseSensitive;
	bool m_bAccentSensitive;
	bool m_bExclude;
	bool m_bDisabled;
	QString m_strPhrase;
	QVariant m_varExtraInfo;	// Extra user info for specific uses of this structure
};

Q_DECLARE_METATYPE(CPhraseEntry)

class CPhraseList : public QList<CPhraseEntry>
{
public:
	inline CPhraseList() : QList<CPhraseEntry>() { }
	inline explicit CPhraseList(const CPhraseEntry &i) : QList<CPhraseEntry>() { append(i); }
	inline CPhraseList(const CPhraseList &l) : QList<CPhraseEntry>(l) { }
	inline CPhraseList &operator =(const CPhraseList &l) { QList<CPhraseEntry>::operator =(l); return *this; }

	int removeDuplicates();
};

Q_DECL_CONST_FUNCTION Q_DECL_PURE_FUNCTION inline uint qHash(const CPhraseEntry &key, uint seed = 0) Q_DECL_NOTHROW
{
	// Note: Aren't hashing "disable" because it doesn't affect the main key value equality
	std::vector<uint> vctHashes = { qHash(key.text()), qHash((key.caseSensitive() ? 4u : 0u) + (key.accentSensitive() ? 2u : 0u) + (key.isExcluded() ? 1u : 0u)) };
	return std::accumulate(vctHashes.begin(), vctHashes.end(), seed, CPhraseEntry::QHashCombineCommutative());
}

// ============================================================================

// Forward declarations:
class TPhraseTag;
class TPhraseTagList;
class TPassageTag;
class TPassageTagList;

// Class to hold the Normalized Lo and Hi indexes covered by a tag
//		with basic manipulations:
class TTagBoundsPair
{
public:
	TTagBoundsPair(uint32_t nNormalLo, uint32_t nNormalHi, bool bHadCount = true);
	TTagBoundsPair(const TTagBoundsPair &tbpSrc);
	TTagBoundsPair(const TPhraseTag &aTag, const CBibleDatabase *pBibleDatabase);

	TTagBoundsPair & operator=(const TTagBoundsPair &src)
	{
		m_pairNormals = src.m_pairNormals;
		m_bHadCount = src.m_bHadCount;
		return *this;
	}

	bool isValid() const { return ((m_pairNormals.first != 0) && (m_pairNormals.second != 0)); }

	inline uint32_t lo() const { return m_pairNormals.first; }
	void setLo(uint32_t nNormal) { m_pairNormals.first = nNormal; }
	inline uint32_t hi() const { return m_pairNormals.second; }
	void setHi(uint32_t nNormal) { m_pairNormals.second = nNormal; }
	inline bool hadCount() const { return m_bHadCount; }
	void setHadCount(bool bHadCount) { m_bHadCount = bHadCount; }

	bool completelyContains(const TTagBoundsPair &tbpSrc) const;
	bool intersects(const TTagBoundsPair &tbpSrc) const;
	bool intersectingInsert(const TTagBoundsPair &tbpSrc);
	bool intersectingTrim(const TTagBoundsPair &tbpSrc);

private:
	typedef QPair<uint32_t, uint32_t> TNormalPair;
	TNormalPair m_pairNormals;
	bool m_bHadCount;							// True if the range had a count of words rather than being a reference to a location without any content
};

// ============================================================================

// Relative Index and Word Count pair used for highlighting phrases:
class TPhraseTag
{
public:
	explicit inline TPhraseTag(const CRelIndex &ndx = CRelIndex(), unsigned int nCount = 0)
		:	m_RelIndex(ndx),
			m_nCount(nCount)
	{ }

	TPhraseTag(const CBibleDatabase *pBibleDatabase, const TTagBoundsPair &tbpSrc);

	TPhraseTag(const CBibleDatabase *pBibleDatabase, const TPassageTag &tagPassage)
		:	m_nCount(0)
	{
		setFromPassageTag(pBibleDatabase, tagPassage);
	}

	inline const CRelIndex &relIndex() const { return m_RelIndex; }
	inline CRelIndex &relIndex() { return m_RelIndex; }						// Needed for >> operator
	inline void setRelIndex(const CRelIndex &ndx) { m_RelIndex = ndx; }
	inline const unsigned int &count() const { return m_nCount; }
	inline unsigned int &count() { return m_nCount; }						// Needed for >> operator
	inline void setCount(unsigned int nCount) { m_nCount = nCount; }

	void setFromPassageTag(const CBibleDatabase *pBibleDatabase, const TPassageTag &tagPassage);
	QString PassageReferenceRangeText(const CBibleDatabase *pBibleDatabase) const;

	bool isSet() const {
		return (m_RelIndex.isSet());
	}

	bool haveSelection() const {
		return ((m_RelIndex.isSet()) && (m_nCount != 0));
	}

	bool operator==(const TPhraseTag &otherTag) const {
		return ((m_RelIndex.index() == otherTag.relIndex().index()) &&
				(m_nCount == otherTag.count()));
	}

	bool operator!=(const TPhraseTag &otherTag) const {
		return ((m_RelIndex.index() != otherTag.relIndex().index()) ||
				(m_nCount != otherTag.count()));
	}

	TTagBoundsPair bounds(const CBibleDatabase *pBibleDatabase) const;			// Returns a pair containing the Normalized Lo and Hi indexes covered by this tag

	bool completelyContains(const CBibleDatabase *pBibleDatabase, const TPhraseTag &aTag) const;
	bool intersects(const CBibleDatabase *pBibleDatabase, const TPhraseTag &aTag) const;
	bool intersectingInsert(const CBibleDatabase *pBibleDatabase, const TPhraseTag &aTag);
	TPhraseTag mask(const CBibleDatabase *pBibleDatabase, const TPhraseTag &aTag) const;		// Creates a new tag that's the content of this tag masked by the specified aTag.
	friend class TPhraseTagList;

private:
	CRelIndex m_RelIndex;
	unsigned int m_nCount;
};
inline QDataStream& operator<<(QDataStream &out, const TPhraseTag &ndx) {
	out << ndx.relIndex() << ndx.count();
	return out;
}
inline QDataStream& operator>>(QDataStream &in, TPhraseTag &ndx) {
	in >> ndx.relIndex() >> ndx.count();
	return in;
}
Q_DECLARE_METATYPE(TPhraseTag)

const QString g_constrPhraseTagMimeType("application/vnd.dewtronics.kjvcanopener.phrasetag");
const QString g_constrHighlighterPhraseTagListMimeType("application/vnd.dewtronics.kjvcanopener.highlighter.phrasetaglist");
const QString g_constrPlainTextMimeType("text/plain");
const QString g_constrHTMLTextMimeType("text/html");

// ----------------------------------------------------------------------------

// List of tags used for highlighting found phrases, etc:
class TPhraseTagList : public QList<TPhraseTag>
{
public:
	TPhraseTagList();
	TPhraseTagList(const TPhraseTag &aTag);
	TPhraseTagList(const TPhraseTagList &src);
	TPhraseTagList(const CBibleDatabase *pBibleDatabase, const TPassageTagList &lstPassageTags);

	bool isSet() const;

	void setFromPassageTagList(const CBibleDatabase *pBibleDatabase, const TPassageTagList &lstPassageTags);

	bool completelyContains(const CBibleDatabase *pBibleDatabase, const TPhraseTag &aTag) const;
	bool completelyContains(const CBibleDatabase *pBibleDatabase, const TPhraseTagList &aTagList) const;
	bool intersects(const CBibleDatabase *pBibleDatabase, const TPhraseTag &aTag) const;
	void intersectingInsert(const CBibleDatabase *pBibleDatabase, const TPhraseTag &aTag);
	void intersectingInsert(const CBibleDatabase *pBibleDatabase, const TPhraseTagList &aTagList);		// Note: Both lists MUST be sorted before calling this function!  The resulting list will be sorted...
	bool removeIntersection(const CBibleDatabase *pBibleDatabase, const TPhraseTag &aTag);
	int findIntersectingIndex(const CBibleDatabase *pBibleDatabase, const TPhraseTag &aTag, int nStartIndex = 0) const;

	bool isEquivalent(const CBibleDatabase *pBibleDatabase, const TPhraseTagList &aTagList) const
	{
		return (completelyContains(pBibleDatabase, aTagList) && aTagList.completelyContains(pBibleDatabase, *this));
	}
};

typedef QList<TPhraseTagList> TPhraseTagListList;		// List of tag lists, use to keep tag lists for multiple phrases

struct TPhraseTagListSortPredicate {
	static bool ascendingLessThan(const TPhraseTag &s1, const TPhraseTag &s2)
	{
		return (s1.relIndex().index() < s2.relIndex().index());
	}
};

struct HighlighterNameSortPredicate {
	bool operator() (const QString &v1, const QString &v2) const;
};

// PhraseTag Highlighter Mapping Types:
typedef std::map<QString, TPhraseTagList, HighlighterNameSortPredicate> THighlighterTagMap;		// Map of HighlighterName to TPhraseTagList (Highlighters are kept in sorted decomposed alphabetical order for overlay order)
typedef std::map<QString, THighlighterTagMap> TBibleDBHighlighterTagMap;						// Map of Bible Database UUID to THighlighterTagMap

// ============================================================================

// Relative Index and VERSE Count pair used for highlighting Passages:

class TPassageTag
{
public:
	explicit inline TPassageTag(const CRelIndex &ndx = CRelIndex(), unsigned int nVerseCount = 0)
		:	m_RelIndex(ndx),
			m_nVerseCount(nVerseCount)
	{
		if (m_RelIndex.isSet()) m_RelIndex.setWord(1);
	}
	TPassageTag(const CBibleDatabase *pBibleDatabase, const TPhraseTag &tagPhrase)
		:	m_nVerseCount(0)
	{
		setFromPhraseTag(pBibleDatabase, tagPhrase);
	}

	inline const CRelIndex &relIndex() const { return m_RelIndex; }
	inline CRelIndex &relIndex() { return m_RelIndex; }							// Needed for >> operator
	inline void setRelIndex(const CRelIndex &ndx) { m_RelIndex = ndx; }
	inline const unsigned int &verseCount() const { return m_nVerseCount; }
	inline unsigned int &verseCount() { return m_nVerseCount; }					// Needed for >> operator
	inline void setVerseCount(unsigned int nVerseCount) { m_nVerseCount = nVerseCount; }

	void setFromPhraseTag(const CBibleDatabase *pBibleDatabase, const TPhraseTag &tagPhrase);
	QString PassageReferenceRangeText(const CBibleDatabase *pBibleDatabase) const;

	bool isSet() const {
		return (m_RelIndex.isSet());
	}

	bool haveSelection() const {
		return ((m_RelIndex.isSet()) && (m_nVerseCount != 0));
	}

	bool operator==(const TPassageTag &otherTag) const {
		return ((m_RelIndex.index() == otherTag.relIndex().index()) &&
				(m_nVerseCount == otherTag.verseCount()));
	}

	bool operator!=(const TPassageTag &otherTag) const {
		return ((m_RelIndex.index() != otherTag.relIndex().index()) ||
				(m_nVerseCount != otherTag.verseCount()));
	}

//	bool completelyContains(const CBibleDatabase *pBibleDatabase, const TPassageTag &aTag) const;
//	bool intersects(const CBibleDatabase *pBibleDatabase, const TPassageTag &aTag) const;
//	bool intersectingInsert(const CBibleDatabase *pBibleDatabase, const TPassageTag &aTag);
	friend class TPassageTagList;

private:
	CRelIndex m_RelIndex;
	unsigned int m_nVerseCount;
};
inline QDataStream& operator<<(QDataStream &out, const TPassageTag &ndx) {
	out << ndx.relIndex() << ndx.verseCount();
	return out;
}
inline QDataStream& operator>>(QDataStream &in, TPassageTag &ndx) {
	in >> ndx.relIndex() >> ndx.verseCount();
	return in;
}
Q_DECLARE_METATYPE(TPassageTag)

const QString g_constrPassageTagMimeType("application/vnd.dewtronics.kjvcanopener.passagetag");

// List of tags used for highlighting found phrases, etc:
class TPassageTagList : public QList<TPassageTag>
{
public:
	TPassageTagList()
		:	QList<TPassageTag>()
	{ }

	TPassageTagList(const TPassageTag &aTag)
		:	QList<TPassageTag>()
	{
		append(aTag);
	}

	TPassageTagList(const TPassageTagList &src)
		:	QList<TPassageTag>(src)
	{ }

	TPassageTagList(const CBibleDatabase *pBibleDatabase, const TPhraseTagList &lstPhraseTags)
		:	QList<TPassageTag>()
	{
		setFromPhraseTagList(pBibleDatabase, lstPhraseTags);
	}

	void setFromPhraseTagList(const CBibleDatabase *pBibleDatabase, const TPhraseTagList &lstPhraseTags);
	unsigned int verseCount() const;

//	bool completelyContains(const CBibleDatabase *pBibleDatabase, const TPassageTag &aTag) const;
//	void intersectingInsert(const CBibleDatabase *pBibleDatabase, const TPassageTag &aTag);
//	bool removeIntersection(const CBibleDatabase *pBibleDatabase, const TPassageTag &aTag);
};

struct TPassageTagListSortPredicate {
	static bool ascendingLessThan(const TPassageTag &s1, const TPassageTag &s2)
	{
		return (s1.relIndex().index() < s2.relIndex().index());
	}
};

// ============================================================================

class CLemmaEntry
{
public:
	CLemmaEntry() { }
	CLemmaEntry(const TPhraseTag &tag, const QString &strLemmaAttrs);
	~CLemmaEntry() { }

	inline TPhraseTag tag() const { return m_tagEntry; }
#ifdef OSIS_PARSER_BUILD
	inline QString lemmaAttrs() const { return m_strLemmaAttrs; }
#endif

	inline int count() const { return m_lstStrongs.size(); }
	inline bool isValid() const
	{
		return ((m_lstStrongs.size() == m_lstText.size()) &&
				(m_lstStrongs.size() == m_lstMorph.size()));
	}

	QString strongs(int nIndex) const;
	const QStringList &strongs() const { return m_lstStrongs; }
	QString text(int nIndex) const;
	const QStringList &text() const { return m_lstText; }
	QString morph(int nIndex) const;
	const QStringList &morph() const { return  m_lstMorph; }
	QString morphSource() const { return  m_strMorphSource; }

private:
	TPhraseTag m_tagEntry;
#ifdef OSIS_PARSER_BUILD
	QString m_strLemmaAttrs;		// Lemma Attributes from OSIS.  These will be parsed into the data below, but this member is only needed during OSIS parsing.
#endif
	// ----
	QStringList m_lstStrongs;		// Array of Strongs Indexes -- count of this is the Lemma count
	QStringList m_lstText;			// Masoretic or Textus-Receptus Words (paired with Strongs Indexes)
	QStringList m_lstMorph;			// Morphological Coding for Words (paired with Strongs Indexes) -- for future expansion for other dictionary sources, we can allow for a comma-separated list
	QString m_strMorphSource;		// Morphological Source: "robinson" or oshm", etc.
};

typedef std::map<CRelIndex, CLemmaEntry, RelativeIndexSortPredicate> TLemmaEntryMap;	// Index by [nBk|nChp|nVrs|nWrd]

// ============================================================================

class CStrongsEntry
{
public:
	CStrongsEntry() { }
	CStrongsEntry(QChar chrLangCode, unsigned int nStrongsIndex)
		:	m_chrLangCode(chrLangCode),
			m_nStrongsIndex(nStrongsIndex)
	{ }
	CStrongsEntry(const QString &strStrongsTextIndex)
		:	m_chrLangCode(!strStrongsTextIndex.isEmpty() ? strStrongsTextIndex.at(0) : '?'),
			m_nStrongsIndex(strStrongsTextIndex.mid(1).toUInt())
	{ }

	QChar langCode() const { return m_chrLangCode; }
	unsigned int strongsIndex() const { return m_nStrongsIndex; }
	QString strongsMapIndex() const { return QString("%1%2").arg(m_chrLangCode).arg(m_nStrongsIndex); }
	QString strongsTextIndex() const { return QString("%1%2").arg(m_chrLangCode).arg(m_nStrongsIndex, 4, 10, QChar('0')); }

	QString orthography() const { return m_strOrthography; }
	QString orthographyPlainText() const { QString strResult = m_strOrthography; return strResult.remove(QRegExp("<[^>]*>")); }
	void setOrthography(const QString &strOrthography) { m_strOrthography = strOrthography; }

	QString transliteration() const { return m_strTransliteration; }
	void setTransliteration(const QString &strTransliteration) { m_strTransliteration = strTransliteration; }

	QString pronunciation() const { return m_strPronunciation; }
	void setPronunciation(const QString &strPronunciation) { m_strPronunciation = strPronunciation; }

	QString definition() const { return m_strDefinition; }
	void setDefinition(const QString &strDefinition) { m_strDefinition = strDefinition; }

private:
	QChar m_chrLangCode;						// Language code: 'G' or 'H'
	unsigned int m_nStrongsIndex = 0;			// Numeric Index
	QString m_strOrthography;					// Word(s) in original language
	QString m_strTransliteration;				// English Transliteration of Word(s)
	QString m_strPronunciation;					// Entry Word(s) Pronunciation.  Note: Will be RichText in some cases as some Hebrew entries have a superscripted 'o' for some reason
	QString m_strDefinition;					// Entry Definition as Rich Text
};

struct StrongsIndexSortPredicate {
	bool operator() (const QString &v1, const QString &v2) const
	{
		if (v1.left(1) < v2.left(1)) return true;
		if (v1.left(1) == v2.left(1)) return (v1.mid(1).toUInt() < v2.mid(1).toUInt());
		return false;
	}
};

typedef std::map<QString, CStrongsEntry, StrongsIndexSortPredicate> TStrongsIndexMap;		// Mapping of StrongsMapIndex to StrongsEntry
typedef QMultiMap<QString, QString> TStrongsOrthographyMap;		// Mapping of Orthography word(s) to StrongsMapIndex -- NOTE: This is a MultiMap, as multiple Strongs Indexes can be mapped to one orthography

// ============================================================================

class TBibleDatabaseSettings
{
public:
	enum HideHyphensOptions {					// <<Bitfields>>
		HHO_None = 0x0,							// Default for no options (i.e. don't hide anything)
		HHO_ProperWords = 0x1,					// Hide Hyphens in "Proper Words"
		HHO_OrdinaryWords = 0x2					// Hide Hyphens in non-"Proper Words"
	};

	explicit TBibleDatabaseSettings()
		:	m_bLoadOnStart(false),
			m_hhoHideHyphens(HHO_None),
			m_bHyphenSensitive(false)
	{ }

	bool isValid() const { return true; }

	inline bool operator==(const TBibleDatabaseSettings &other) const {
		return ((m_bLoadOnStart == other.m_bLoadOnStart) &&
				(m_hhoHideHyphens == other.m_hhoHideHyphens) &&
				(m_bHyphenSensitive == other.m_bHyphenSensitive));
	}
	inline bool operator!=(const TBibleDatabaseSettings &other) const {
		return (!operator==(other));
	}

	bool loadOnStart() const { return m_bLoadOnStart; }
	void setLoadOnStart(bool bLoadOnStart) { m_bLoadOnStart = bLoadOnStart; }

	unsigned int hideHyphens() const { return m_hhoHideHyphens; }
	void setHideHyphens(unsigned int nHHO) { m_hhoHideHyphens = nHHO; }

	bool hyphenSensitive() const { return m_bHyphenSensitive; }
	void setHyphenSensitive(bool bHyphenSensitive) { m_bHyphenSensitive = bHyphenSensitive; }

private:
	bool m_bLoadOnStart;
	unsigned int m_hhoHideHyphens;
	bool m_bHyphenSensitive;
};

typedef QMap<QString, TBibleDatabaseSettings> TBibleDatabaseSettingsMap;		// Map of Bible UUIDs to settings for saving/preserving

// ============================================================================

class CReadDatabase;			// Forward declaration for class friendship
class COSISXmlHandler;

class CVerseTextRichifierTags;
class CKJPBSWordScriptureObject;
class QAbstractTextDocumentLayout;

// CBibleDatabase - Class to define a Bible Database file
class CBibleDatabase
{
private:
	CBibleDatabase(const TBibleDescriptor &bblDesc);		// Creatable by CReadDatabase
public:
	~CBibleDatabase();

	TBibleDatabaseSettings settings() const;
	void setSettings(const TBibleDatabaseSettings &aSettings);

	BibleTypeOptionsFlags flags() const { return m_btoFlags; }
	QString language() const { return m_strLanguage; }
	QString name() const { return m_strName; }
	QString description() const { return m_strDescription; }
	QString info() const { return m_strInfo; }
	QString compatibilityUUID() const { return m_strCompatibilityUUID; }
	QString highlighterUUID() const { return m_strHighlighterUUID; }

	QString translatedColophonString() const;				// Text "Colophon"
	QString translatedSuperscriptionString() const;			// Text "Superscription"

	bool hasColophons() const;								// Returns true if any book has colophons in this Bible
	bool hasSuperscriptions() const;						// Returns true if any chapter has superscriptions in this Bible

	bool completelyContains(const TPhraseTag &aPhraseTag) const;		// Returns true if this Bible database completely contains the specified tag (i.e. none of it lies outside the database text)
	TTagBoundsPair bounds() const;

	TPhraseTag bookPhraseTag(const CRelIndex &nRelIndex) const;
	TPhraseTag chapterPhraseTag(const CRelIndex &nRelIndex) const;
	TPhraseTag versePhraseTag(const CRelIndex &nRelIndex) const;

	void registerTextLayoutHandlers(QAbstractTextDocumentLayout *pDocLayout);

#ifdef USING_WEBCHANNEL
	QString toJsonBkChpStruct() const;		// Generate Book/Chapter Structure as JSON for WebChannel
#endif

	// CRelIndex Name/Report Functions:
	QString SearchResultToolTip(const CRelIndex &nRelIndex, unsigned int nRIMask = RIMASK_ALL, unsigned int nSelectionSize = 1) const;		// Create complete reference statistics report
	QString PassageReferenceText(const CRelIndex &nRelIndex, bool bSuppressWordOnPseudoVerse = false) const;		// Creates a reference text string like "Genesis 1:1 [5]"
	QString PassageReferenceAbbrText(const CRelIndex &nRelIndex, bool bSuppressWordOnPseudoVerse = false) const;	// Creates a reference abbreviated text string like "Gen 1:1 [5]"

	QString testamentName(const CRelIndex &nRelIndex) const;
	uint32_t testament(const CRelIndex &nRelIndex) const;

	QString bookCategoryName(const CRelIndex &nRelIndex) const;
	uint32_t bookCategory(const CRelIndex &nRelIndex) const;
	int bookCategoryCount() const
	{
		return m_lstBookCategories.size();
	}

	QString bookName(const CRelIndex &nRelIndex) const;
	QString bookNameAbbr(const CRelIndex &nRelIndex) const;
	QString bookOSISAbbr(const CRelIndex &nRelIndex) const;

	// CRelIndex Transformation Functions:
#ifdef OSIS_PARSER_BUILD
	uint32_t NormalizeIndexNoAccum(const CRelIndex &ndxRelIndex) const;
	CRelIndex DenormalizeIndexNoAccum(uint32_t nNormalIndex) const;
#endif
	uint32_t NormalizeIndex(const CRelIndex &ndxRelIndex) const;
	CRelIndex DenormalizeIndex(uint32_t nNormalIndex) const;
#ifdef USE_EXTENDED_INDEXES
	uint32_t NormalizeIndexEx(const CRelIndexEx &ndxRelIndex) const;
	CRelIndexEx DenormalizeIndexEx(uint32_t nNormalIndex) const;
#endif

	// calcRelIndex - Calculates a relative index from counts.  For example, starting from (0,0,0,0):
	//			calcRelIndex(1, 1, 666, 0, 1);						// Returns (21,7,1,1) or Ecclesiastes 7:1 [1], Word 1 of Verse 1 of Chapter 666 of the Bible
	//			calcRelIndex(1, 393, 0, 5, 0);						// Returns (5, 13, 13, 1) or Deuteronomy 13:13 [1], Word 1 of Verse 393 of Book 5 of the Bible
	//			calcRelIndex(1, 13, 13, 5, 0);						// Returns (5, 13, 13, 1) or Deuteronomy 13:13 [1], Word 1 of Verse 13 of Chapter 13 of Book 5 of the Bible
	//			calcRelIndex(1, 13, 13, 5, 1);						// Returns (5, 13, 13, 1) or Deuteronomy 13:13 [1], Word 1 of Verse 13 of Chapter 13 of Book 5 of the Old Testament
	//			calcRelIndex(1, 13, 13, 5, 2);						// Returns (44, 13, 13, 1) or Acts 13:13 [1], Word 1 of Verse 13 of Chapter 13 of Book 5 of the New Testament
	//			calcRelIndex(0, 13, 13, 5, 2);						// Returns (44, 13, 13, 1) or Acts 13:13 [1], Word 1 of Verse 13 of Chapter 13 of Book 5 of the New Testament
	CRelIndex calcRelIndex(
					unsigned int nWord, unsigned int nVerse, unsigned int nChapter,
					unsigned int nBook, unsigned int nTestament,
					const CRelIndex &ndxStart = CRelIndex(),
					bool bReverse = false) const;

	// Note: Changing the following will require updating and redeploying
	//		of the WebChannel pages, in addition to CKJVBrowser, etc:
	enum RELATIVE_INDEX_MOVE_ENUM {
		RIME_Absolute = 0,				// Move to Absolute Index or NoMove (default entry for doing no relative calculation except for checking validity and renormalizing to next valid location)
		RIME_Start = 1,					// Move to Beginning of the Bible
		RIME_StartOfBook = 2,			// Move to Beginning of current Book
		RIME_StartOfChapter = 3,		// Move to Beginning of current Chapter
		RIME_StartOfVerse = 4,			// Move to Beginning of current Verse
		RIME_End = 5,					// Move to Ending of the Bible
		RIME_EndOfBook = 6,				// Move to Ending of current Book
		RIME_EndOfChapter = 7,			// Move to Ending of current Chapter
		RIME_EndOfVerse = 8,			// Move to Ending of current Verse
		RIME_PreviousBook = 9,			// Move Backward one Book (to the start of it)
		RIME_PreviousChapter = 10,		// Move Backward one Chapter (to the start of it)
		RIME_PreviousVerse = 11,		// Move Backward one Verse (to the start of it)
		RIME_PreviousWord = 12,			// Move Backward one Word
		RIME_NextBook = 13,				// Move Forward one Book (to the start of it)
		RIME_NextChapter = 14,			// Move Forward one Chapter (to the start of it)
		RIME_NextVerse = 15,			// Move Forward one Verse (to the start of it)
		RIME_NextWord = 16				// Move Forward one Word
	};
	CRelIndex calcRelIndex(const CRelIndex &ndxStart, RELATIVE_INDEX_MOVE_ENUM nMoveMode) const;	// Calculates new index as per MoveMode.  Returns 0 (Not Set) if result is invalid, such as PreviousBook from Genesis 1, for example.

	unsigned int bookWordCountProper(unsigned int nBook) const;
	unsigned int chapterWordCountProper(unsigned int nBook, unsigned int nChapter) const;

	inline const CBibleEntry &bibleEntry() const						// Bible stats entry
	{
		return m_EntireBible;
	}
	const CTestamentEntry *testamentEntry(uint32_t nTst) const;			// Testament stats/data entry
	const CBookCategoryEntry *bookCategoryEntry(uint32_t nCat) const;	// Category stats/data entry
	const CBookEntry *bookEntry(uint32_t nBk) const;					// Book Data or Table of Contents [Book]
	const CBookEntry *bookEntry(const CRelIndex &ndx) const;			// Book Data or Table of Contents Use CRelIndex:[Book | 0 | 0 | 0]
#ifdef OSIS_PARSER_BUILD
	const CChapterEntry *chapterEntry(const CRelIndex &ndx, bool bForceCreate = false) const;		// Chapter Data Use CRelIndex:[Book | Chapter | 0 | 0]
	const CVerseEntry *verseEntry(const CRelIndex &ndx, bool bForceCreate = false) const;			// Verse Data Entry Use CRelIndex:[Book | Chapter | Verse | 0]
#else
	const CChapterEntry *chapterEntry(const CRelIndex &ndx) const;		// Chapter Data Use CRelIndex:[Book | Chapter | 0 | 0]
	const CVerseEntry *verseEntry(const CRelIndex &ndx) const;			// Verse Data Entry Use CRelIndex:[Book | Chapter | Verse | 0]
#endif
	const CWordEntry *wordlistEntry(const QString &strWord) const;		// WordList Data Entry: Index by lowercase keyword
	inline const TWordListMap &mapWordList() const						// Master word-list Map
	{
		return m_mapWordList;
	}
	inline const QStringList &lstWordList() const						// List version of Master word-list (used for quick matching)
	{
		return m_lstWordList;
	}
	inline const TConcordanceList &concordanceWordList() const			// List of all words as composed UTF8 in sorted order.  Used for index mapping, lookup and searching
	{
		return m_lstConcordanceWords;
	}
	void setRenderedWords();
	void setRenderedWords(CWordEntry &aWordEntry) const;
	int concordanceIndexForWordAtIndex(uint32_t ndxNormal) const;			// Returns the concordanceWordList() index for the Word at the specified Bible Normalized Index (or -1 if not found)
	int concordanceIndexForWordAtIndex(const CRelIndex &relIndex) const;	// Returns the concordanceWordList() index for the Word at the specified Bible Normalized Index (or -1 if not found)
	const CConcordanceEntry *concordanceEntryForWordAtIndex(uint32_t ndxNormal) const;			// Returns the CConcordanceEntry object for the word at the specified index -- like concordanceIndexForWordAtIndex, but returns underlying CConcordanceEntry
	const CConcordanceEntry *concordanceEntryForWordAtIndex(const CRelIndex &relIndex) const;	// Returns the CConcordanceEntry object for the word at the specified index -- like concordanceIndexForWordAtIndex, but returns underlying CConcordanceEntry
	QString wordAtIndex(uint32_t ndxNormal, bool bAsRendered = true) const;				// Returns word of the Bible based on Normalized Index (1 to Max) -- Automatically does ConcordanceMapping Lookups -- If bAsRendered=true, applies dehyphen to remove hyphens based on settings
	QString wordAtIndex(const CRelIndex &relIndex, bool bAsRendered = true) const;		// Returns word of the Bible based on Relative Index (Denormalizes and calls wordAtIndex() for normal above) -- If bAsRendered=true, applies dehyphen to remove hyphens based on settings
	QString decomposedWordAtIndex(uint32_t ndxNormal) const;			// Returns word of the Bible (decomposed) based on Normalized Index (1 to Max) -- Automatically does ConcordanceMapping Lookups
	const CFootnoteEntry *footnoteEntry(const CRelIndex &ndx) const;	// Footnote Data Entry, Used CRelIndex:[Book | Chapter | Verse | Word], for unused, set to 0, example: [1 | 1 | 0 | 0] for Genesis 1 (See TFootnoteEntryMap above)
	inline const TFootnoteEntryMap &footnotesMap() const				// Entire Footnote Map, needed for database generation
	{
		return m_mapFootnotes;
	}
	inline const CPhraseList &phraseList() const						// Returns the Common Phrases List from the Main Database for this Bible Database
	{
		return m_lstCommonPhrases;
	}
	const CLemmaEntry *lemmaEntry(const CRelIndex &ndx) const;			// Lemma Data Entry, Used CRelIndex: [Book | Chapter | Verse | Word], supports Colophons and Superscription indexes
	inline const TLemmaEntryMap &lemmaMap() const						// Entire Lemma Map, needed for database generation
	{
		return m_mapLemmaEntries;
	}
	const CStrongsEntry *strongsEntryByIndex(const QString &strIndex) const;		// Strongs Entry by Map Index (G1, H22, etc);
	QList<const CStrongsEntry *> strongsEntriesByOthography(const QString &strOrth) const;	// Strongs Entries from Orthographic Word
	QStringList strongsIndexesFromOrthograph(const QString &strOrth) const;		// Lookup Orthographic Word and return a List of Strongs Map Indexes
	inline const TStrongsIndexMap &strongsIndexMap() const
	{
		return m_mapStrongsEntries;
	}
	inline const TStrongsOrthographyMap &strongsOrthographyMap() const
	{
		return m_mapStrongsOrthographyMap;
	}
	QString soundEx(const QString &strDecomposedConcordanceWord, bool bCache = true) const;		// Return and/or calculate soundEx for the specified Concordance Word (calculations done based on this Bible Database language)

	QString richVerseText(const CRelIndex &ndxRel,
							const CVerseTextRichifierTags &tags,
							bool bAddAnchors = false,
							const CBasicHighlighter *aHighlighter = NULL,
							bool bUseLemmas = false) const;	// Generate and return verse text for specified index: [Book | Chapter | Verse | 0]
#ifdef BIBLE_DATABASE_RICH_TEXT_CACHE
	void dumpRichVerseTextCache(uint nTextRichifierTagHash = 0);		// Dump the cache for a specific CVerseTextRichifierTags object (pass its hash) or all data (pass 0)
#endif

private:
	// CReadDatabase needed to load the database.  After that everything
	//	is read-only.  Database building is done directly from the CSV files
	//
	friend class CReadDatabase;
	friend class COSISXmlHandler;			// COSISXmlHandler - Used by KJVDataParse for OSIS XML File processing to build KJPBS databases

// Main Database Data:
	CBibleEntry m_EntireBible;				// Entire Bible stats, calculated from testament stats in ReadDB.
	TTestamentList m_lstTestaments;			// Testament List: List(nTst-1)
	TBookCategoryList m_lstBookCategories;	// Category List: List(nCat-1)
	TBookList m_lstBooks;					// Books (Table of Contents): List(nBk-1)
	TChapterMap m_mapChapters;				// Chapter Entries Map: Map(CRelIndex[nBk | nChp | 0 | 0])
	TBookVerseList m_lstBookVerses;			// Book Verse Entries List: List(nBk-1) -> Map(CRelIndex[nBk | nChp | nVrs | 0])
	TWordListMap m_mapWordList;				// Master word-list Map (Indexed by lowercase word)
	QStringList m_lstWordList;				// Master word-list List as lowercase, used for searching lower/upper-bound for m_mapWordList
	TConcordanceList m_lstConcordanceWords;	// List (QStringList) of all Unique Words as Composed UTF8 in the order for the concordance with names of the TWordListMap key (starts at index 0)
	TNormalizedIndexList m_lstConcordanceMapping;	// List of WordNdx# (in ConcordanceWords) for all 789629 words of the text (starts at index 1)
	TFootnoteEntryMap m_mapFootnotes;		// Footnotes (typed by index - See notes above with TFootnoteEntryMap)
	CPhraseList m_lstCommonPhrases;			// Common phrases read from database
	TLemmaEntryMap m_mapLemmaEntries;		// Lemmas (typed by index - See notes above with TLemmaEntryMap)
	TStrongsIndexMap m_mapStrongsEntries;	// Strongs Entries mapped by StrongsMapIndex
	TStrongsOrthographyMap m_mapStrongsOrthographyMap;		// Map of Strongs Orthography word(s) to StrongsMapIndex
	mutable TSoundExMap m_mapSoundEx;		// SoundEx map of Decomposed words (from m_lstConcordanceWords) to SoundEx equivalent, used to minimize calculations

// Local Data:
	BibleTypeOptionsFlags m_btoFlags;		// Bible Database Flags
	QString m_strLanguage;					// Language ID for this database (en, es, etc)
	QString m_strName;						// Name for this database
	QString m_strDescription;				// Database description
	QString m_strInfo;						// Information about this database (copyright details, etc)
	QString m_strCompatibilityUUID;			// Unique Identifier inside database that data can be tied to to know that the database has the same word count structure such that highlighters and things still work
	QString m_strHighlighterUUID;			// Master Highlighter UUID for which this database is compatible with

	CKJPBSWordScriptureObject *m_pKJPBSWordScriptureObject;		// Object used to render the words from this database in the Scripture Editor/Browser

// Cache:
#ifdef BIBLE_DATABASE_RICH_TEXT_CACHE
	mutable TSpecVerseCacheMap m_mapVerseCacheWithAnchors;		// Map of Verse Cache Maps to store rendered rich text if g_nRichTextCachingMode is as RTCME_FULL
	mutable TSpecVerseCacheMap m_mapVerseCacheNoAnchors;		//  "(ditto)" (but without anchors)
#endif
};

Q_DECLARE_METATYPE(CBibleDatabase *)
typedef QSharedPointer<CBibleDatabase> CBibleDatabasePtr;
Q_DECLARE_METATYPE(CBibleDatabasePtr)

class  TBibleDatabaseList : public QObject, protected QList<CBibleDatabasePtr>
{
	Q_OBJECT

private:				// Enforce Singleton:
	TBibleDatabaseList(QObject *pParent = NULL);

public:
	virtual ~TBibleDatabaseList();
	static TBibleDatabaseList *instance();

#ifdef USING_WEBCHANNEL
	static QString availableBibleDatabasesAsJson();
#endif
	static bool loadBibleDatabase(BIBLE_DESCRIPTOR_ENUM nBibleDB, bool bAutoSetAsMain = false, QWidget *pParent = NULL);
	static bool loadBibleDatabase(const QString &strUUID, bool bAutoSetAsMain = false, QWidget *pParent = NULL);
	CBibleDatabasePtr mainBibleDatabase() const { return m_pMainBibleDatabase; }
	void setMainBibleDatabase(const QString &strUUID);
	bool haveMainBibleDatabase() const { return (!m_pMainBibleDatabase.isNull()); }
	void removeBibleDatabase(const QString &strUUID);
	void clear();
	int size() const { return QList<CBibleDatabasePtr>::size(); }
	CBibleDatabasePtr at(int i) const { return QList<CBibleDatabasePtr>::at(i); }
	CBibleDatabasePtr atUUID(const QString &strUUID) const;

	QList<BIBLE_DESCRIPTOR_ENUM> availableBibleDatabases();		// List of BDEs of available Bible Databases
	QStringList availableBibleDatabasesUUIDs();					// List of UUIDs of available Bible Databases
	void findBibleDatabases();

protected:
	friend class CReadDatabase;
	void addBibleDatabase(CBibleDatabasePtr pBibleDatabase, bool bSetAsMain);			// Added via CReadDatabase

signals:
	void loadedBibleDatabase(CBibleDatabasePtr pBibleDatabase);
	void removingBibleDatabase(CBibleDatabasePtr pBibleDatabase);
	void changedMainBibleDatabase(CBibleDatabasePtr pBibleDatabase);
	void changedBibleDatabaseList();
	void changedAvailableBibleDatabaseList();

protected slots:
	void en_changedBibleDatabaseSettings(const QString &strUUID, const TBibleDatabaseSettings &aSettings);

private:
	CBibleDatabasePtr m_pMainBibleDatabase;
	bool m_bHaveSearchedAvailableDatabases;							// True when we've done at least one find operation
	QList<BIBLE_DESCRIPTOR_ENUM> m_lstAvailableDatabases;			// List of descriptor enums for Bible databases available
};

// ============================================================================

// Dictionary Word Entry -- Mapping of words and their Definitions:
//

class CDictionaryWordEntry : public CBasicWordEntry
{
public:
	CDictionaryWordEntry();
	CDictionaryWordEntry(const QString &strWord);
	virtual ~CDictionaryWordEntry() { }

	CDictionaryWordEntry & operator=(const CDictionaryWordEntry &src)
	{
		m_strWord = src.m_strWord;
		m_strDecomposedWord = src.m_strDecomposedWord;
		m_lstDefinitions = src.m_lstDefinitions;
		m_lstIndexes = src.m_lstIndexes;
		return *this;
	}

	virtual const QString &word() const { return m_strWord; }
	virtual const QString &decomposedWord() const { return m_strDecomposedWord; }
	virtual const QString &decomposedHyphenWord() const { return m_strDecomposedWord; }
	virtual const QString &deApostrWord() const { return m_strDecomposedWord; }
	virtual const QString &deApostrHyphenWord() const { return m_strDecomposedWord; }
	virtual const QString &renderedWord() const { return m_strWord; }
	const QStringList &definitions() const { return m_lstDefinitions; }
	const QList<int> &indexes() const { return m_lstIndexes; }
	void addDefinition(int nIndex, const QString &strDefinition)
	{
		if (!m_lstIndexes.contains(nIndex)) {
			m_lstIndexes.append(nIndex);
			m_lstDefinitions.append(strDefinition);
		}
	}

	bool operator==(const CDictionaryWordEntry &src) const
	{
		return (m_strDecomposedWord.compare(src.m_strDecomposedWord) == 0);
	}
	bool operator!=(const CDictionaryWordEntry &src) const
	{
		return (m_strDecomposedWord.compare(src.m_strDecomposedWord) != 0);
	}

private:
	QString m_strWord;						// Composed Word (as in the actual text)
	QString m_strDecomposedWord;			// Lowercase Decomposed Word (used for matching)
	QStringList m_lstDefinitions;			// Rich-Text Definitions of the Word
	QList<int> m_lstIndexes;				// Database indexes -- used for live database lookup
};

typedef std::map<QString, CDictionaryWordEntry> TDictionaryWordListMap;		// Indexed by lower-case decomposed words from word-list

// ============================================================================

class TDictionaryDatabaseSettings
{
public:
	explicit TDictionaryDatabaseSettings()
		:	m_bLoadOnStart(false)
	{ }

	bool isValid() const { return true; }

	inline bool operator==(const TDictionaryDatabaseSettings &other) const {
		return (m_bLoadOnStart == other.m_bLoadOnStart);
	}
	inline bool operator!=(const TDictionaryDatabaseSettings &other) const {
		return (!operator==(other));
	}

	bool loadOnStart() const { return m_bLoadOnStart; }
	void setLoadOnStart(bool bLoadOnStart) { m_bLoadOnStart = bLoadOnStart; }

private:
	bool m_bLoadOnStart;
};

typedef QMap<QString, TDictionaryDatabaseSettings> TDictionaryDatabaseSettingsMap;		// Map of Dictionary UUIDs to settings for saving/preserving

// ============================================================================

// CDictionaryDatabase - Class to define a Dictionary Database file
class CDictionaryDatabase
{
private:
	CDictionaryDatabase(const TDictionaryDescriptor &dctDesc);		// Creatable by CReadDatabase
public:
	~CDictionaryDatabase();

	DictionaryTypeOptionsFlags flags() const { return m_dtoFlags; }
	QString language() const { return m_strLanguage; }
	QString name() const { return m_strName; }
	QString description() const { return m_strDescription; }
	QString info() const { return m_strInfo; }
	QString compatibilityUUID() const { return m_strCompatibilityUUID; }
	bool isLiveDatabase() const {
#ifndef NOT_USING_SQL
		return m_myDatabase.isOpen();
#else
		return false;
#endif
	}

	QString soundEx(const QString &strDecomposedDictionaryWord, bool bCache = true) const;		// Return and/or calculate soundEx for the specified Dictionary Word (calculations done based on this Dictionary Database language)

	QString definition(const QString &strWord) const;		// Lookup and return definition for word
	bool wordExists(const QString &strWord) const;			// Lookup word and return true/false on its existence

	inline const TDictionaryWordListMap &mapWordList() const { return m_mapWordDefinitions; }
	inline const QStringList &lstWordList() const { return m_lstWordList; }

private:
	// CReadDatabase needed to load the database.  After that everything
	//	is read-only.
	//
	friend class CReadDatabase;

// Main Database Data:
	TDictionaryWordListMap m_mapWordDefinitions;
	QStringList m_lstWordList;				// List of decomposed lower-case keywords from map, stored in list for quick enumeration
	mutable TSoundExMap m_mapSoundEx;		// SoundEx map of Decomposed words (from m_mapWordDefinitions) to SoundEx equivalent, used to minimize calculations

// Local Data:
	DictionaryTypeOptionsFlags m_dtoFlags;	// Dictionary Database Flags
	QString m_strLanguage;					// Language ID for this database (en, es, etc)
	QString m_strName;						// Name for this database
	QString m_strDescription;				// Database description
	QString m_strInfo;						// Information about this database (copyright details, etc)
	QString m_strCompatibilityUUID;			// Unique Identifier inside database that data can be tied to to know that the database has the same word count structure such that highlighters and things still work
#ifndef NOT_USING_SQL
	QSqlDatabase m_myDatabase;				// Open SQL for this dictionary
#endif
};

Q_DECLARE_METATYPE(CDictionaryDatabase *)
typedef QSharedPointer<CDictionaryDatabase> CDictionaryDatabasePtr;
Q_DECLARE_METATYPE(CDictionaryDatabasePtr)

class  TDictionaryDatabaseList : public QObject, protected QList<CDictionaryDatabasePtr>
{
	Q_OBJECT

private:				// Enforce Singleton:
	TDictionaryDatabaseList(QObject *pParent = NULL);

public:
	virtual ~TDictionaryDatabaseList();
	static TDictionaryDatabaseList *instance();

	static CDictionaryDatabasePtr locateAndLoadDictionary(const QString &strLanguage, QWidget *pParentWidget = NULL);		// Locates and loads the best candidate dictionary for the specified language based on MainDictionary and DictionaryLoad settings

	static bool loadDictionaryDatabase(DICTIONARY_DESCRIPTOR_ENUM nDictDB, bool bAutoSetAsMain = false, QWidget *pParent = NULL);
	static bool loadDictionaryDatabase(const QString &strUUID, bool bAutoSetAsMain = false, QWidget *pParent = NULL);
	CDictionaryDatabasePtr mainDictionaryDatabase() const { return m_pMainDictionaryDatabase; }
	void setMainDictionaryDatabase(const QString &strUUID);
	bool haveMainDictionaryDatabase() const { return (!m_pMainDictionaryDatabase.isNull()); }
	void removeDictionaryDatabase(const QString &strUUID);
	void clear();
	int size() const { return QList<CDictionaryDatabasePtr>::size(); }
	CDictionaryDatabasePtr at(int i) const { return QList<CDictionaryDatabasePtr>::at(i); }
	CDictionaryDatabasePtr atUUID(const QString &strUUID) const;

	QList<DICTIONARY_DESCRIPTOR_ENUM> availableDictionaryDatabases();	// List of DDEs of available Dictionary Databases
	QStringList availableDictionaryDatabasesUUIDs();					// List of UUIDs of available Dictionary Databases
	void findDictionaryDatabases();

protected:
	friend class CReadDatabase;
	void addDictionaryDatabase(CDictionaryDatabasePtr pDictionaryDatabase, bool bSetAsMain);		// Added via CReadDatabase

signals:
	void loadedDictionaryDatabase(CDictionaryDatabasePtr pDictionaryDatabase);
	void removingDictionaryDatabase(CDictionaryDatabasePtr pDictionaryDatabase);
	void changedMainDictionaryDatabase(CDictionaryDatabasePtr pDictionaryDatabase);
	void changedDictionaryDatabaseList();
	void changedAvailableDictionaryDatabaseList();

private:
	CDictionaryDatabasePtr m_pMainDictionaryDatabase;
	bool m_bHaveSearchedAvailableDatabases;							// True when we've done at least one find operation
	QList<DICTIONARY_DESCRIPTOR_ENUM> m_lstAvailableDatabases;		// List of descriptor enums for Dictionary databases available
};


// ============================================================================

#endif // DBSTRUCT_H
