// MQ2SpellSearch.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.

/*
* Used to filter spells to get a list.
* Example: /spellsearch MinLevel 100 MaxLevel 115 Category 125 IgnoreRank
* would output a list to the mq2chatwnd (or console) that met the above requirements
*/

#include <mq/Plugin.h>
#include <MQ2SpellSearch.h>

PreSetup("MQ2SpellSearch");
PLUGIN_VERSION(0.1);

using namespace mq::datatypes;

struct SpellSearch
{
	private:

		// Default values of filter settings.
		const bool defCanScribe				= false;
		const bool defShowSpellEffects		= false;

		// Only spells the char can use are shown unless true.
		const bool defShowAll				= false;
		const bool defShowMissingSpellsOnly = false;
		const bool defShowReverse			= false;
		const bool defShowLastRecord		= false;
		const bool defShowFirstRecord		= false;

		const bool defShowDetailedOutput	= false;
		const bool defSpellRecordGiven		= false;
		const int  defVectorRecord			= 0;

		// Only show spells that are relevant to pchar classlevel.
		const bool defIgnoreClass			= false;

		// Not sure of the intention.  I'm assuming that the rank
		// need not be shown. "spell", "spell rk. II", "spell rk. III"
		// should just show "spell", screening duplicates.
		const bool defIgnoreRank			= false;

		const bool defDebug					= false;

		int eEQSPArev[sizeof(eEQSPA)];

	public:

		// User supplied args
		std::string RawQuery			= "";

		// Spell Data
		int         ID					= -1;
		std::string Name				= "";
		std::string PartialName			= "";
		std::string Class				= "";
		int         MinLevel			= 1;
		int         MaxLevel			= 1;
		std::string Category			= "";
		int         nCategory			= -1;
		std::string SubCategory			= "";
		int         nSubCategory		= -1;
		std::string SubCategory2		= "";
		int			nSubCategory2		= -1;
		int			SpellClass			= -1;
		int			SpellSubClass		= -1;
		int			SpellGroup			= -1;
		int			SpellSubGroup		= -1;

		int         Timer				= -1;
		bool		SpellRecordGiven	= defSpellRecordGiven;
		bool		IgnoreClass			= defIgnoreClass;
		bool		ShowAll				= defShowAll;
		int			Reflectable			= -1;
		int			Feedbackable		= -1;
		float		Range				= -1;
		float		AERange				= -1;
		float		Pushback			= -1;
		int			HateGenerated		= -1;
		int			TargetType			= -1;
		int			NumEffects			= -1;
		int			Skill				= -1;

		// SPA will turn into a vector struct - not fully implemented.
		// This will allow multiple spell effects to be part of the query.
		int SPA = -1;

		// Configuration settings - filters
		bool CanScribe				= defCanScribe;
		bool ShowSpellEffects		= defShowSpellEffects;
		bool ShowMissingSpellsOnly	= defShowMissingSpellsOnly;
		bool ShowFirstRecord		= defShowFirstRecord;
		bool ShowLastRecord			= defShowLastRecord;
		bool ShowReverse			= defShowReverse;
		bool ShowDetailedOutput		= defShowDetailedOutput;
		int  VectorRecord			= defVectorRecord;
		bool IgnoreRank				= defIgnoreRank;
		bool Debug					= defDebug;

		// How many were shown to the user with these view criteria?
		int SpellsShown = 0;

		// Allow comparison of SpellSearch objects. Data only / params given by user, not looked up like nCategory.
		bool SpellSearch::operator==(const SpellSearch& pOther) const
		{
			if (
				ID					== pOther.ID &&
				Name				== pOther.Name &&
				PartialName			== pOther.PartialName &&
				Class				== pOther.Class &&
				MinLevel			== pOther.MinLevel &&
				MaxLevel			== pOther.MaxLevel &&
				Category			== pOther.Category &&
				SubCategory			== pOther.SubCategory &&
				SubCategory2		== pOther.SubCategory2 &&
				SpellClass			== pOther.SpellClass &&
				SpellSubClass		== pOther.SpellSubClass &&
				SpellGroup			== pOther.SpellGroup &&
				SpellSubGroup		== pOther.SpellSubGroup &&

				Timer				== pOther.Timer &&
				SpellRecordGiven	== pOther.SpellRecordGiven &&
				IgnoreClass			== pOther.IgnoreClass &&
				ShowAll				== pOther.ShowAll &&

				Reflectable			== pOther.Reflectable &&
				Feedbackable		== pOther.Feedbackable &&
				Range				== pOther.Range &&
				AERange				== pOther.AERange &&
				Pushback			== pOther.Pushback &&
				HateGenerated		== pOther.HateGenerated &&
				TargetType			== pOther.TargetType &&
				NumEffects			== pOther.NumEffects &&
				Skill				== pOther.Skill
				)
				return true;
			return false;
		}

		// Allow comparison of SpellSearch objects. Data only.
		bool SpellSearch::operator!=(const SpellSearch& pOther) const
		{
			if (
				ID					!= pOther.ID ||
				Name				!= pOther.Name ||
				PartialName			!= pOther.PartialName ||
				Class				!= pOther.Class ||
				MinLevel			!= pOther.MinLevel ||
				MaxLevel			!= pOther.MaxLevel ||
				Category			!= pOther.Category ||
				SubCategory			!= pOther.SubCategory ||
				SubCategory2		!= pOther.SubCategory2 ||
				SpellClass			!= pOther.SpellClass ||
				SpellSubClass		!= pOther.SpellSubClass ||
				SpellGroup			!= pOther.SpellGroup ||
				SpellSubGroup		!= pOther.SpellSubGroup ||

				Timer				!= pOther.Timer ||
				SpellRecordGiven	!= pOther.SpellRecordGiven ||
				IgnoreClass			!= pOther.IgnoreClass ||
				ShowAll				!= pOther.ShowAll ||

				Reflectable			!= pOther.Reflectable ||
				Feedbackable		!= pOther.Feedbackable ||
				Range				!= pOther.Range ||
				AERange				!= pOther.AERange ||
				Pushback			!= pOther.Pushback ||
				HateGenerated		!= pOther.HateGenerated ||
				TargetType			!= pOther.TargetType ||
				NumEffects			!= pOther.NumEffects ||
				Skill				!= pOther.Skill
				)
				return true;
			return false;
		}

		void CacheData(const SpellSearch& pOther)
		{
			// User supplied args
			RawQuery			= pOther.RawQuery;

			// Data
			ID					= pOther.ID;
			Name				= pOther.Name;
			PartialName			= pOther.PartialName;
			Class				= pOther.Class;
			MinLevel			= pOther.MinLevel;
			MaxLevel			= pOther.MaxLevel;
			Category			= pOther.Category;
			nCategory			= pOther.nCategory;
			SubCategory			= pOther.SubCategory;
			nSubCategory		= pOther.nSubCategory;
			SubCategory2		= pOther.SubCategory2;
			nSubCategory2		= pOther.nSubCategory2;
			SpellClass			= pOther.SpellClass;
			SpellSubClass		= pOther.SpellSubClass;
			SpellGroup			= pOther.SpellGroup;
			SpellSubGroup		= pOther.SpellSubGroup;

			Timer				= pOther.Timer;
			SpellRecordGiven	= pOther.SpellRecordGiven;
			IgnoreClass			= pOther.IgnoreClass;
			ShowAll				= pOther.ShowAll;

			Reflectable			= pOther.Reflectable;
			Feedbackable		= pOther.Feedbackable;
			Range				= pOther.Range;
			AERange				= pOther.AERange;
			Pushback			= pOther.Pushback;
			HateGenerated		= pOther.HateGenerated;
			TargetType			= pOther.TargetType;
			NumEffects			= pOther.NumEffects;
			Skill				= pOther.Skill;
		}

		void CacheView(const SpellSearch& pOther)
		{
			// View
			CanScribe				= pOther.CanScribe;
			ShowSpellEffects		= pOther.ShowSpellEffects;
			ShowMissingSpellsOnly	= pOther.ShowMissingSpellsOnly;
			ShowReverse				= pOther.ShowReverse;
			ShowFirstRecord			= pOther.ShowFirstRecord;
			ShowLastRecord			= pOther.ShowLastRecord;
			ShowDetailedOutput		= pOther.ShowDetailedOutput;
			VectorRecord			= pOther.VectorRecord;
			IgnoreRank				= pOther.IgnoreRank;
			Debug					= pOther.Debug;
		}

		void Clear()
		{
			RawQuery			= "";

			ID					= -1;
			Name				= "";
			PartialName			= "";
			Class				= "";
			MinLevel			= 1;
			MaxLevel			= 1;
			Category			= "";
			nCategory			= -1;
			SubCategory			= "";
			nSubCategory		= -1;
			SubCategory2		= "";
			nSubCategory2		= -1;
			SpellClass			= -1;
			SpellSubClass		= -1;
			SpellGroup			= -1;
			SpellSubGroup		= -1;

			Timer				= -1;
			SpellRecordGiven	= defSpellRecordGiven;
			IgnoreClass			= defIgnoreClass;
			ShowAll				= defShowAll;

			Reflectable			= -1;
			Feedbackable		= -1;
			Range				= -1;
			AERange				= -1;
			Pushback			= -1;
			HateGenerated		= -1;
			TargetType			= -1;
			NumEffects			= -1;
			Skill				= -1;

			// PH this will be converted into a struct
			SPA = -1;

			CanScribe				= defCanScribe;
			ShowSpellEffects		= defShowSpellEffects;
			ShowMissingSpellsOnly	= defShowMissingSpellsOnly;
			ShowReverse				= defShowReverse;
			ShowFirstRecord			= defShowFirstRecord;
			ShowLastRecord			= defShowLastRecord;
			ShowDetailedOutput		= defShowDetailedOutput;
			VectorRecord			= defVectorRecord;
			IgnoreRank				= defIgnoreRank;
			Debug					= defDebug;

			// How many were shown to the user.
			SpellsShown = 0;
		}
};

class MQ2SpellSearchType : public MQ2Type
{
public:
	MQ2SpellSearchType();

	/*
		Helpers
	*/
	bool MQ2SpellSearchType::GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) override;
	std::vector<PSPELL> MQ2SpellSearchType::FindSpells(SpellSearch& psSpellSearch, bool isCMD);
	bool MQ2SpellSearchType::KnowSpell(int SpellID);
	SpellSearch MQ2SpellSearchType::ParseSpellSearch(const char* Buffer);
	char* MQ2SpellSearchType::ParseSpellSearchArgs(char* szArg, char* szRest, SpellSearch& psSpellSearch);
	int MQ2SpellSearchType::GetVectorRecordID(SpellSearch& psSearchSpells, std::vector<PSPELL>& pvMatchList);
	bool MQ2SpellSearchType::OutputFilter(SpellSearch& psSearchSpells, PSPELL& thisSpell);


	/*
		CMD
	*/
	void MQ2SpellSearchType::SpellSearchCmd(PlayerClient* pChar, char* szLine);

	/*
		TLO
	*/
	bool MQ2SpellSearchType::GetSpellSearchState(std::string_view query);
	static bool MQ2SpellSearchType::dataSpellSearch(const char* szIndex, MQTypeVar& Ret);

	/*
		Output
	*/
	void MQ2SpellSearchType::OutputResultsCMD(SpellSearch& psSpellSearch, std::vector<PSPELL>& pvMatchList);
	void MQ2SpellSearchType::OutputResultConsole(SpellSearch& psSearchSpells, PSPELL& pthisSpell);
	void MQ2SpellSearchType::DumpPSpellMembers(PSPELL& pSpell);
	void MQ2SpellSearchType::ShowCMDHelp();

};
MQ2SpellSearchType* pSpellSearchType = nullptr;

struct SPELLSEARCHELEMENT
{
	public:

		int id = -1;
		std::string name = "";
		int level = -1;
		int category = -1;
		int subcategory = -1;
		int subcategory2 = -1;
		int spellclass = -1;
		int spellsubclass = -1;
		int spellgroupid = -1;
		int spellsubgroupid = -1;
		int recordID = -1;

		std::string rawquery = "";
		std::string trimmedquery = "";
		std::string query = "";
		std::string previousquery = "";

		SpellSearch SearchSpells;

		std::vector<PSPELL> vMatchesList;
		int spellsfound = 0;

		void Clear()
		{
			id = -1;
			name = "";
			level = -1;
			category = -1;
			subcategory = -1;
			subcategory2 = -1;
			spellclass = -1;
			spellsubclass = -1;
			spellgroupid = -1;
			spellsubgroupid = -1;
			recordID =-1;

			rawquery = "";
			trimmedquery = "";
			query = "";
			previousquery = "";

			vMatchesList.clear();
			spellsfound = 0;
		}
};
SPELLSEARCHELEMENT* pSpellSearch = new SPELLSEARCHELEMENT;

enum class SpellSearchMembers
{
	ID = 1,
	Name,
	Level,
	Category,
	SubCategory,
	SubCategory2,
	SpellClass,
	SpellSubClass,
	SpellGroup,
	SpellSubGroup,
	Count,
	RecordID,
	Query,
};

MQ2SpellSearchType::MQ2SpellSearchType() : MQ2Type("SpellSearch")
{
	ScopedTypeMember(SpellSearchMembers, ID);
	ScopedTypeMember(SpellSearchMembers, Name);
	ScopedTypeMember(SpellSearchMembers, Level);
	ScopedTypeMember(SpellSearchMembers, Category);
	ScopedTypeMember(SpellSearchMembers, SubCategory);
	ScopedTypeMember(SpellSearchMembers, SubCategory2);
	ScopedTypeMember(SpellSearchMembers, SpellClass);
	ScopedTypeMember(SpellSearchMembers, SpellSubClass);
	ScopedTypeMember(SpellSearchMembers, SpellGroup);
	ScopedTypeMember(SpellSearchMembers, SpellSubGroup);
	ScopedTypeMember(SpellSearchMembers, Count);
	ScopedTypeMember(SpellSearchMembers, RecordID);
	ScopedTypeMember(SpellSearchMembers, Query);
}

/*
	Command wrapper to make mq:fEQCommand happy
*/
void SpellSearchCmd(PlayerClient *pChar, char *szLine)
{
	MQ2SpellSearchType().SpellSearchCmd(pChar, szLine);
}

/*
	Helpers
*/

bool MQ2SpellSearchType::GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest)
{
	PcProfile* pProfile = GetPcProfile();
	if (!pProfile) return false;

	MQTypeMember* pMember = MQ2SpellSearchType::FindMember(Member);
	if (!pMember) return false;

	switch (static_cast<SpellSearchMembers>(pMember->ID))
	{
	case SpellSearchMembers::ID:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			Dest.DWord = pSpellSearch->id;
			Dest.Type = pIntType;
			return true;
		}
		return false;

	case SpellSearchMembers::Name:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			strcpy_s(DataTypeTemp, pSpellSearch->name.c_str());
			Dest.Ptr = &DataTypeTemp[0];
			Dest.Type = pStringType;
			return true;
		}
		return false;

	case SpellSearchMembers::Level:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			Dest.DWord = pSpellSearch->level;
			Dest.Type = pIntType;
			return true;
		}
		return false;

	case SpellSearchMembers::Category:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			Dest.DWord = pSpellSearch->category;
			Dest.Type = pIntType;
			return true;
		}
		return false;

	case SpellSearchMembers::SubCategory:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			Dest.DWord = pSpellSearch->subcategory;
			Dest.Type = pIntType;
			return true;
		}
		return false;

	case SpellSearchMembers::SubCategory2:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			Dest.DWord = pSpellSearch->subcategory2;
			Dest.Type = pIntType;
			return true;
		}
		return false;

	case SpellSearchMembers::SpellClass:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			Dest.DWord = pSpellSearch->spellclass;
			Dest.Type = pIntType;
			return true;
		}
		return false;

	case SpellSearchMembers::SpellSubClass:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			Dest.DWord = pSpellSearch->spellsubclass;
			Dest.Type = pIntType;
			return true;
		}
		return false;

	case SpellSearchMembers::SpellGroup:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			Dest.DWord = pSpellSearch->spellgroupid;
			Dest.Type = pIntType;
			return true;
		}
		return false;

	case SpellSearchMembers::SpellSubGroup:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			Dest.DWord = pSpellSearch->spellsubgroupid;
			Dest.Type = pIntType;
			return true;
		}
		return false;

	case SpellSearchMembers::Count:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			Dest.DWord = pSpellSearch->spellsfound;
			Dest.Type = pIntType;
		}
		else
		{
			Dest.DWord = 0;
			Dest.Type = pIntType;
		}
		return true;

	case SpellSearchMembers::RecordID:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			Dest.DWord = pSpellSearch->recordID;
			Dest.Type = pIntType;
			return true;
		}
		return false;

	case SpellSearchMembers::Query:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			strcpy_s(DataTypeTemp, pSpellSearch->query.c_str());
			Dest.Ptr = &DataTypeTemp[0];
			Dest.Type = pIntType;
			return true;
		}
		return false;

	default:

		break;
	}

	return false;
}

char* MQ2SpellSearchType::ParseSpellSearchArgs(char* szArg, char* szRest, SpellSearch& psSpellSearch)
{
	if (szArg)
	{
		// Ideally, debug is the very first parameter.
		// fetch the next arg and continue.
		if (!_stricmp(szArg, "Debug") || !_stricmp(szArg, "-debug"))
		{
			psSpellSearch.Debug = true;
			return szRest;
		}

		// Allow a spell id to be passed without a flag unless its been set already.
		if (atoi(szArg) > 0 && psSpellSearch.ID == -1)
		{
			psSpellSearch.ID = atoi(szArg);
			return szRest;
		}

		if ((!_stricmp(szArg, "ID") || !_stricmp(szArg, "-id")) && psSpellSearch.ID == -1)
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.ID = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "Name") || !_stricmp(szArg, "-name") || !_stricmp(szArg, "-n"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.Name = szArg;
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "PartialName") || !_stricmp(szArg, "-partialname") || !_stricmp(szArg, "pname") || !_stricmp(szArg, "-pname") || !_stricmp(szArg, "-pn"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.PartialName = szArg;
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "Category") || !_stricmp(szArg, "-category") || !_stricmp(szArg, "Cat") || !_stricmp(szArg, "-cat"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.Category = szArg;
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "SubCategory") || !_stricmp(szArg, "-subcategory") || !_stricmp(szArg, "SubCat") || !_stricmp(szArg, "-subcat"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.SubCategory = szArg;
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "SubCategory2") || !_stricmp(szArg, "-subcategory2") || !_stricmp(szArg, "SubCat2") || !_stricmp(szArg, "-subcat2"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.SubCategory2 = szArg;
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "SpellClass") || !_stricmp(szArg, "-spellclass"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.SpellClass = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "SpellSubClass") || !_stricmp(szArg, "-spellsubclass"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.SpellSubClass = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "SpellGroup") || !_stricmp(szArg, "-spellgroup"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.SpellGroup = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "SpellSubGroup") || !_stricmp(szArg, "-spellsubgroup"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.SpellSubGroup = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "MinLevel") || !_stricmp(szArg, "-minlevel") || !_stricmp(szArg, "-minl"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.MinLevel = atoi(szArg);
			if (psSpellSearch.MinLevel < 1) psSpellSearch.MinLevel = 1;
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "MaxLevel") || !_stricmp(szArg, "-maxlevel") || !_stricmp(szArg, "-maxl"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.MaxLevel = atoi(szArg);
			if (psSpellSearch.MaxLevel < psSpellSearch.MinLevel) psSpellSearch.MaxLevel = psSpellSearch.MinLevel;
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "TargetType") || !_stricmp(szArg, "-targettype"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.TargetType = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "Timer") || !_stricmp(szArg, "-timer"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.Timer = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		// This probably needs to go into a vector array as multiple criteria could be specified.
		if (!_stricmp(szArg, "SPA") || !_stricmp(szArg, "-spa"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.SPA = atoi(szArg);
			WriteChatf("SPA set to %i", psSpellSearch.SPA);
			return GetNextArg(szRest, 1);
		}

		// Flag
		if (!_stricmp(szArg, "IgnoreRank") || !_stricmp(szArg, "-ignorerank"))
		{
			psSpellSearch.IgnoreRank = true;
			return szRest;
		}

		// Flag
		if (!_stricmp(szArg, "IgnoreClass") || !_stricmp(szArg, "-ignoreclass"))
		{
			psSpellSearch.IgnoreClass = true;
			return szRest;
		}

		// Flag
		if (!_stricmp(szArg, "Scribable") || !_stricmp(szArg, "-scribable"))
		{
			psSpellSearch.CanScribe = true;
			return szRest;
		}

		// Flag
		if (!_stricmp(szArg, "ShowSpellEffects") || !_stricmp(szArg, "-showspelleffects") || !_stricmp(szArg, "-sse"))
		{
			psSpellSearch.ShowSpellEffects = true;
			return szRest;
		}

		// Flag
		if (!_stricmp(szArg, "ShowDetailedOutput") || !_stricmp(szArg, "-showdetailedoutput") || !_stricmp(szArg, "-sdo"))
		{
			psSpellSearch.ShowDetailedOutput = true;
			return szRest;
		}

		// The intention of -all is to return all matching spells from 1 to whatever the game maxlevel is.
		// If -all, and missing flag is off, it is interpreted as "all spells I can have whether I know them or not."
		// If -all and missing, it is interpreted as "all spells I don't know"
		// If all and missing are not specific, it is interpreted as "all spells I have"
		if (!_stricmp(szArg, "ShowAll") || !_stricmp(szArg, "-showall") || !_stricmp(szArg, "all") || !_stricmp(szArg, "-all"))
		{
			psSpellSearch.ShowAll = true;
			return szRest;
		}

		// Flag: Reverse output order. This works logically with -record
		if (!_stricmp(szArg, "Reverse") || !_stricmp(szArg, "-reverse") || !_stricmp(szArg, "-rev"))
		{
			psSpellSearch.ShowReverse = true;
			return szRest;
		}

		// View - if it finds a number then it assumes thats the spell you want in the list
		// A specified spell will not be filtered any further.
		if (!_stricmp(szArg, "Record") || !_stricmp(szArg, "-record") ||
			!_stricmp(szArg, "Show") || !_stricmp(szArg, "-show")
			)
		{
			GetArg(szArg, szRest, 1);
			if (!_stricmp(szArg, "last"))
			{
				psSpellSearch.ShowLastRecord = true;
			}
			else if (!_stricmp(szArg, "first"))
			{
				psSpellSearch.ShowFirstRecord = true;
			}
			// this is an alternative to using -missing, unless you need to specify a particular record.
			else if (!_stricmp(szArg, "missing"))
			{
				psSpellSearch.ShowMissingSpellsOnly = true;
			}
			else
			{
				psSpellSearch.VectorRecord = atoi(szArg);
				// Treat as first record.
				if (psSpellSearch.VectorRecord < 0)
				{
					psSpellSearch.VectorRecord = 0;
					psSpellSearch.ShowFirstRecord = true;
				}
				// Treat as asking for the last record.
				if (psSpellSearch.VectorRecord > sizeof(pSpellMgr->Spells))
				{
					psSpellSearch.VectorRecord = 0;
					psSpellSearch.ShowLastRecord = true;
				}
				if (psSpellSearch.VectorRecord > 0) psSpellSearch.SpellRecordGiven = true;
			}
			return GetNextArg(szRest, 1);
		}

		// Default behavior is that only spells a character has will be shown unless -all is also
		// given as a parameter.
		if (!_stricmp(szArg, "Missing") || !_stricmp(szArg, "-missing") || !_stricmp(szArg, "-miss"))
		{
			psSpellSearch.ShowMissingSpellsOnly = true;
			return szRest;
		}

		if (!_stricmp(szArg, "Reflectable") || !_stricmp(szArg, "-reflectable") || !_stricmp(szArg, "-reflect"))
		{
			GetArg(szArg, szRest, 1);
			if (!_stricmp(szArg, "yes") || !_stricmp(szArg, "true") || !_stricmp(szArg, "1"))
			{
				psSpellSearch.Reflectable = 1;
			}
			else if (!_stricmp(szArg, "no") || !_stricmp(szArg, "false") || !_stricmp(szArg, "0"))
			{
				psSpellSearch.Reflectable = 0;
			}
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "Feedbackable") || !_stricmp(szArg, "-feedbackable") || !_stricmp(szArg, "-feedback"))
		{
			GetArg(szArg, szRest, 1);
			if (!_stricmp(szArg, "yes") || !_stricmp(szArg, "true") || !_stricmp(szArg, "1"))
			{
				psSpellSearch.Feedbackable = 1;
			}
			else if (!_stricmp(szArg, "no") || !_stricmp(szArg, "false") || !_stricmp(szArg, "0"))
			{
				psSpellSearch.Feedbackable = 0;
			}
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "Skill") || !_stricmp(szArg, "-skill"))
		{
			GetArg(szArg, szRest, 1);

			// This returns a 0 if not found, which is the same as 1h bash....
			psSpellSearch.Skill = GetSkillIDFromName(szArg);

			if (psSpellSearch.Debug)
			{
				WriteChatf("DEBUG :: Skill %i  %s", psSpellSearch.Skill, szArg);
			}
			return GetNextArg(szRest, 1);
		}

		// If we get here, then we did not find a matching parameter. Let's assume its the spell unless
		// it or partialname have been set. If they enclosed the name with quotes, then it will all be in
		// szArg which will loop until done.
		if (szArg[0] != 0 && psSpellSearch.PartialName == "")
		{
			psSpellSearch.Name += szArg;
			psSpellSearch.Name += " ";
		}
	}
	return szRest;
}

SpellSearch MQ2SpellSearchType::ParseSpellSearch(const char* Buffer)
{
	SpellSearch psSpellSearch;

	pSpellSearch->SearchSpells.RawQuery = Buffer;

	char szArg[MAX_STRING] = { 0 };
	char szMsg[MAX_STRING] = { 0 };
	char szLLine[MAX_STRING] = { 0 };
	char* szFilter = szLLine;
	bool bArg = true;

	strcpy_s(szLLine, Buffer);
	_strlwr_s(szLLine);

	while (bArg)
	{
		GetArg(szArg, szFilter, 1);
		szFilter = GetNextArg(szFilter, 1);

		if (pSpellSearch->SearchSpells.Debug)
		{
			WriteChatf("DEBUG :: ParseSpellSearch :: szArg    : %s", szArg);
			WriteChatf("DEBUG :: ParseSpellSearch :: szFilter : %s", szFilter);
		}

		if (szArg[0] == 0)
		{
			bArg = false;
		}
		else
		{
			szFilter = ParseSpellSearchArgs(szArg, szFilter, psSpellSearch);
		}
	}

	if (psSpellSearch.ShowFirstRecord && psSpellSearch.ShowLastRecord)
	{
		// They negate, so turn them off. Otherwise the power grid will fail.
		psSpellSearch.ShowFirstRecord = false;
		psSpellSearch.ShowLastRecord = false;
	}

	return psSpellSearch;
}

// If TLO, then WriteChatf is suppressed.
std::vector<PSPELL> MQ2SpellSearchType::FindSpells(SpellSearch& psSearchSpells, bool isCMD)
{
	/*
	if (psSearchSpells.Debug)
	{
		WriteChatf("DEBUG :: FindSpells: Skill %i", psSearchSpells.Skill);
	}
	*/

	std::vector<PSPELL> pvMatchList;
	PSPELL thisSpell = nullptr;

	// Look for our exact match criteria: ID and Name
	if (psSearchSpells.ID != -1)
	{
		thisSpell = pSpellMgr->GetSpellByID(psSearchSpells.ID);
		if (thisSpell == nullptr)
		{
			if (isCMD)
			{
				WriteChatf("\aw[MQ2SpellSearch] \aySpell ID %i not found.", psSearchSpells.ID);
			}
			return pvMatchList;
		}

		pvMatchList.push_back(thisSpell);
		return pvMatchList;
	}

	if (!string_equals(psSearchSpells.Name, ""))
	{
		trim(psSearchSpells.Name);
		thisSpell = GetSpellByName(psSearchSpells.Name.c_str());
		if (thisSpell == nullptr)
		{
			if (isCMD)
			{
				WriteChatf("\aw[MQ2SpellSearch] \aySpell Name [%s] not found.", psSearchSpells.Name.c_str());
			}
			return pvMatchList;
		}
		pvMatchList.push_back(thisSpell);
		return pvMatchList;
	}

	// Look up the category and subcategory ids
	int SpellCat = 0;
	int SpellSubCat = 0;
	int SpellSubCat2 = 0;

	// e.g. Pet or 69
	if (!string_equals(psSearchSpells.Category, ""))
	{
		trim(psSearchSpells.Category);
		psSearchSpells.nCategory = atoi(psSearchSpells.Category.c_str());
		SpellCat = psSearchSpells.nCategory;

		if (!SpellCat)
		{
			SpellCat = (int)GetSpellCategoryFromName(psSearchSpells.Category.c_str());
		}

		if (!SpellCat)
		{
			if (isCMD)
			{
				WriteChatf("\aw[MQ2SpellSearch] \ayCould not find Category %s", psSearchSpells.Category.c_str());
			}
			return pvMatchList;
		}
	}

	// e.g. Sum: Earth or 100
	if (!string_equals(psSearchSpells.SubCategory, ""))
	{
		trim(psSearchSpells.SubCategory);
		psSearchSpells.nSubCategory = atoi(psSearchSpells.SubCategory.c_str());
		SpellSubCat = psSearchSpells.nSubCategory;

		if (!SpellSubCat)
		{
			SpellSubCat = (int)GetSpellCategoryFromName(psSearchSpells.SubCategory.c_str());
		}

		if (!SpellSubCat)
		{
			if (isCMD)
			{
				WriteChatf("\aw[MQ2SpellSearch] \ayCould not find SubCategory %s", psSearchSpells.SubCategory.c_str());
			}
			return pvMatchList;
		}
	}

	// Unsure, but putting it in since it looks important.
	if (!string_equals(psSearchSpells.SubCategory2, ""))
	{
		trim(psSearchSpells.SubCategory2);
		psSearchSpells.nSubCategory2 = atoi(psSearchSpells.SubCategory2.c_str());
		SpellSubCat2 = psSearchSpells.nSubCategory;

		if (!SpellSubCat2)
		{
			SpellSubCat2 = (int)GetSpellCategoryFromName(psSearchSpells.SubCategory2.c_str());
		}

		if (!SpellSubCat2)
		{
			if (isCMD)
			{
				WriteChatf("\aw[MQ2SpellSearch] \ayCould not find SubCategory2 %s", psSearchSpells.SubCategory2.c_str());
			}
			return pvMatchList;
		}
	}

	// Look for spells that contain given criteria

	int NextHighestLevelSpellID = 0;
	int MaxLevelSpellID = 0;
	int NextHighestLevel = 0;
	int MaxLevel = 0;

	// We check this after the for loop.
	int NumParams = 0;
	int iSrchCat = 0;
	int iSrchSubCat = 0;
	int iSrchSubCat2 = 0;
	int iSrchSpellClass = 0;
	int iSrchSpellSubClass = 0;
	int iSrchSpellGroup = 0;
	int iSrchSpellSubGroup = 0;
	int iSrchLevel = 0;
	int iSrchPartialName = 0;
	int iSrchFeedback = 0;
	int iSrchReflectable = 0;
	int iSrchTargetType = 0;
	int iSrchSkill = 0;

	int iSpells = sizeof(pSpellMgr->Spells);

	if ((psSearchSpells.IgnoreClass || psSearchSpells.PartialName.length() > 0) && (!SpellCat && (!SpellSubCat || !SpellSubCat2)))
	{
		if (isCMD)
		{
			WriteChatf("\aw[MQ2SpellSearch] \ayYour search time may be very.. long. Preferably, specify Category and SubCategory. Please wait.");
		}
	}

	// Look up spells given the criteria
	for (int x = 0; x <= iSpells; ++x)
	{
		// Reset the search counters
		NumParams = 0;
		iSrchCat = 0;
		iSrchSubCat = 0;
		iSrchSubCat2 = 0;
		iSrchSpellClass = 0;
		iSrchSpellSubClass = 0;
		iSrchSpellGroup = 0;
		iSrchSpellSubGroup = 0;
		iSrchLevel = 0;
		iSrchPartialName = 0;
		iSrchFeedback = 0;
		iSrchReflectable = 0;
		iSrchTargetType = 0;
		iSrchSkill = 0;

		thisSpell = pSpellMgr->GetSpellByID(x);
		if (thisSpell == nullptr)
		{
			if (isCMD)
			{
				WriteChatf("\aw[MQ2SpellSearch] \arProblem initializing search. Try reloading plugin, mq, or restart Windows.");
			}
			return pvMatchList;
		}

		if (thisSpell->ID == 0) continue;

		int ClassLevel = thisSpell->ClassLevel[GetPcProfile()->Class];

		if (!psSearchSpells.IgnoreClass && ClassLevel > 253) continue;

		if (string_equals(thisSpell->Name, "NPCSpellPlaceholder")) continue;
		if (string_equals(thisSpell->Name, "AVCReserved")) continue;
		if (string_equals(thisSpell->Name, "AVC Reserved")) continue;

		if (SpellCat)
		{
			iSrchCat = 1;
			NumParams++;
			if (thisSpell->Category != SpellCat) continue;
		}

		if (SpellSubCat)
		{
			iSrchSubCat = 1;
			NumParams++;
			if (thisSpell->Subcategory != SpellSubCat) continue;
		}

		if (SpellSubCat2)
		{
			iSrchSubCat2 = 1;
			NumParams++;
			if (thisSpell->Subcategory2 != SpellSubCat2) continue;
		}

		if (psSearchSpells.SpellClass != -1)
		{
			iSrchSpellClass = 1;
			NumParams++;
			if (thisSpell->SpellClass != psSearchSpells.SpellClass) continue;
		}

		if (psSearchSpells.SpellSubClass != -1)
		{
			iSrchSpellSubClass = 1;
			NumParams++;
			if (thisSpell->SpellSubClass != psSearchSpells.SpellSubClass) continue;
		}

		if (psSearchSpells.SpellGroup != -1)
		{
			iSrchSpellGroup = 1;
			NumParams++;
			if (thisSpell->SpellGroup != psSearchSpells.SpellGroup) continue;
		}

		if (psSearchSpells.SpellSubGroup != -1)
		{
			iSrchSpellSubGroup = 1;
			NumParams++;
			if (thisSpell->SpellSubGroup != psSearchSpells.SpellSubGroup) continue;
		}

		if (psSearchSpells.Debug)
		{
			WriteChatf("DEBUG:: Spell skill: %i Parameter: %i", (int)thisSpell->Skill, psSearchSpells.Skill);
		}

		if (psSearchSpells.Skill != -1)
		{
			iSrchSkill = 1;
			NumParams++;
			if ((int)thisSpell->Skill != psSearchSpells.Skill) continue;
		}

		if (psSearchSpells.TargetType != -1)
		{
			iSrchTargetType = 1;
			NumParams++;
			if (thisSpell->TargetType != psSearchSpells.TargetType) continue;
		}

		if (psSearchSpells.Reflectable != -1)
		{
			iSrchReflectable = 1;
			NumParams++;
			if (thisSpell->Reflectable != (bool)psSearchSpells.Reflectable) continue;
		}

		if (psSearchSpells.Feedbackable != -1)
		{
			iSrchFeedback = 1;
			NumParams++;
			if (thisSpell->Feedbackable != (bool)psSearchSpells.Feedbackable) continue;
		}

		// If we ignore class, then level is meaningless.
		if (!psSearchSpells.IgnoreClass)
		{
			if (psSearchSpells.MinLevel > 1 || psSearchSpells.MaxLevel > 1)
			{
				iSrchLevel = 1;
				NumParams++;
				int MinLevelValue = 1;
				int MaxLevelValue = 1;

				if (psSearchSpells.ShowAll)
				{
					int MinLevelValue = 1;
					int MaxLevelValue = 255;
				}
				else
				{
					MinLevelValue = psSearchSpells.MinLevel > 1 ? psSearchSpells.MinLevel : 1;
					MaxLevelValue = psSearchSpells.MaxLevel > 1 ? psSearchSpells.MaxLevel : GetPcProfile()->Level;
				}

				if (ClassLevel < MinLevelValue || ClassLevel > MaxLevelValue) continue;
			}
		}
		// Should this require at least one of the spell categories? It's going to be slow.
		if (!string_equals(psSearchSpells.PartialName, ""))
		{
			NumParams++;
			iSrchPartialName = 1;
			int iPosition = ci_find_substr(thisSpell->Name, psSearchSpells.PartialName);
			//WriteChatf("[%s] ?= [%s] (%i)", thisSpell->Name, SearchSpells.PartialName, iPosition);
			if (iPosition < 0) continue;
		}

		//bool IsSPAEffect(EQ_Spell* pSpell, int EffectID)
		//int GetSpellAttrib(EQ_Spell* pSpell, int index)
		// 

		// We searched all the parameters we specified. If we are here, we found something.
		if (NumParams == (	iSrchPartialName +
							iSrchCat +
							iSrchSubCat +
							iSrchSubCat2 +
							iSrchSpellClass +
							iSrchSpellSubClass +
							iSrchSpellGroup +
							iSrchSpellSubGroup +
							iSrchLevel +
							iSrchSkill +
							iSrchFeedback +
							iSrchReflectable +
							iSrchTargetType
												))
		{
			pvMatchList.push_back(thisSpell);
		}
	}

	// Sort by spell level - some spells were entered into the spell db at funky record positions.
	sort(pvMatchList.begin(), pvMatchList.end(), [](PSPELL a, PSPELL b)
		{
			return (a->ClassLevel[GetPcProfile()->Class] < b->ClassLevel[GetPcProfile()->Class]);
		}
	);

	if (psSearchSpells.Debug)
	{
		PSPELL tempSpell;
		int vSize = pvMatchList.size();

		WriteChatf("\n\agDEBUG :: Found %i spells", vSize);
		for (int i = 0; i < vSize; ++i)
		{
			tempSpell = pvMatchList.at(i);

			WriteChatf("DEBUG :: Record: %i ID: %i Name: %s ClsLev: %i", i, tempSpell->ID, tempSpell->Name, tempSpell->ClassLevel[GetPcProfile()->Class]);
		}
		WriteChatf("\n");
	}
	return pvMatchList;
}

// Example of finding a spell in the spellbook
/*
EQ_Spell* GetHighestLearnedSpellByGroupID(int dwSpellGroupID)
{
	PcProfile* pProfile = GetPcProfile();
	if (!pProfile) return nullptr;

	EQ_Spell* result = nullptr;

	for (int nSpell : pProfile->SpellBook)
	{
		auto pFoundSpell = GetSpellByID(nSpell);
		if (!pFoundSpell || pFoundSpell->SpellGroup != dwSpellGroupID)
			continue;

		// Find the highest rank of the spell that matches this spell group
		if (!result || result->SpellRank < pFoundSpell->SpellRank)
			result = pFoundSpell;
	}

	return result;
}
*/

bool MQ2SpellSearchType::KnowSpell(int SpellID)
{
	PcProfile* pProfile = GetPcProfile();
	if (!pProfile) return false;

	for (int nSpell : pProfile->SpellBook)
	{
		if (nSpell == SpellID) return true;
	}
	return false;
}

int MQ2SpellSearchType::GetVectorRecordID(SpellSearch& psSearchSpells, std::vector<PSPELL>& pvMatchList)
{
	if (!psSearchSpells.ShowFirstRecord && !psSearchSpells.ShowLastRecord && !psSearchSpells.SpellRecordGiven)
	{
		return 1;
	}

	// "first" condition
	if (psSearchSpells.ShowFirstRecord) return 1;

	int szvMatchList = pvMatchList.size();

	// "last" condition
	if (psSearchSpells.ShowLastRecord) return szvMatchList;
	
	if (psSearchSpells.VectorRecord > szvMatchList) return szvMatchList;

	// This record must be specified.
	return psSearchSpells.VectorRecord;
}

/*
	Output
*/

bool MQ2SpellSearchType::OutputFilter(SpellSearch& psSearchSpells, PSPELL& thisSpell)
{
	// This is more clearly coded to make it readable. Hopefully.
	// If the id or name of a specific spell was specified, then show that record.
	if (psSearchSpells.ID != -1 || !string_equals(psSearchSpells.Name, "")) return true;

	// If the filter is on, pass through. If it's off, then check the spell. Either level is unimportant, or it is important and we pass through.

	if (!psSearchSpells.IgnoreClass)
	{
		if (thisSpell->ClassLevel[GetPcProfile()->Class] > 253) return false;
	}

	if (!psSearchSpells.ShowAll)
	{
		if (psSearchSpells.ShowMissingSpellsOnly)
		{
			if (KnowSpell(thisSpell->ID) || thisSpell->ClassLevel[GetPcProfile()->Class] > GetPcProfile()->Level) return false;
		}
		else
		{
			if (!KnowSpell(thisSpell->ID) || thisSpell->ClassLevel[GetPcProfile()->Class] > GetPcProfile()->Level) return false;
		}
	}
	else
	{
		if (psSearchSpells.ShowMissingSpellsOnly)
		{
			if (KnowSpell(thisSpell->ID)) return false;
		}
	}

	/*
	// Not sure how to implement this.
	if (psSearchSpells.IgnoreRank)
	{
		
	}
	*/

	// If we get here then the spell met criteria.
	return true;
}

/*
	CMD
*/

void MQ2SpellSearchType::SpellSearchCmd(PlayerClient* pChar, char* szLine)
{
	if (strlen(szLine) == 0 || !_stricmp(szLine, "help") || !_stricmp(szLine, "-h") || !_stricmp(szLine, "-help"))
	{
		ShowCMDHelp();
		return;
	}

	SpellSearch psSearchSpells = ParseSpellSearch(szLine);

	if (psSearchSpells == pSpellSearch->SearchSpells)
	{
		if (psSearchSpells.Debug)
		{
			WriteChatf("DEBUG :: Same query, different view");
		}

		pSpellSearch->SearchSpells.CacheView(psSearchSpells);
	}
	else
	{
		if (psSearchSpells.Debug)
		{
			WriteChatf("DEBUG :: New query");
		}

		pSpellSearch->SearchSpells.CacheData(psSearchSpells);
		pSpellSearch->SearchSpells.CacheView(psSearchSpells);
		std::vector<PSPELL>& lvMatchesList = FindSpells(pSpellSearch->SearchSpells, true);
		pSpellSearch->vMatchesList = lvMatchesList;
		pSpellSearch->spellsfound = lvMatchesList.size();
	}

	if (pSpellSearch->SearchSpells.Debug)
	{
		WriteChatf("DEBUG :: Spells Found: %i", pSpellSearch->spellsfound);
	}

	if (pSpellSearch->spellsfound > 0) OutputResultsCMD(pSpellSearch->SearchSpells, pSpellSearch->vMatchesList);

	if (pSpellSearch->SearchSpells.Debug)
	{
		WriteChatf("DEBUG :: Spells Shown: %i", pSpellSearch->SearchSpells.SpellsShown);
	}

	if (!pSpellSearch->SearchSpells.SpellsShown)
	{
		WriteChatf("\aw[MQ2SpellSearch] \ayNo matches found");
	}
	else
	{
		WriteChatf("\aw[MQ2SpellSearch] \aoFound %d matches", pSpellSearch->SearchSpells.SpellsShown);
	}
}

void MQ2SpellSearchType::OutputResultsCMD(SpellSearch& psSearchSpells, std::vector<PSPELL>& pvMatchList)
{
	PcProfile* pProfile = GetPcProfile();
	if (!pProfile) return;

	PSPELL thisSpell;

	psSearchSpells.SpellsShown = 0;
	int szvMatchList = pvMatchList.size();

	// Copy vector over
	std::vector<PSPELL> vTempList;
	for (int i = 0; i < szvMatchList; ++i)
	{
		if (OutputFilter(psSearchSpells, pvMatchList.at(i))) vTempList.push_back(pvMatchList.at(i));
	}

	szvMatchList = vTempList.size();

	if (psSearchSpells.ShowReverse)
	{
		sort(vTempList.begin(), vTempList.end(), [](PSPELL a, PSPELL b)
			{
				return (a->ClassLevel[GetPcProfile()->Class] > b->ClassLevel[GetPcProfile()->Class]);
			}
		);
	}

	// Retrieves starting point
	int RecordID = GetVectorRecordID(psSearchSpells, vTempList);
	if (!RecordID) return;

	// need a function to retrieve ending point?
	if (psSearchSpells.ShowFirstRecord) szvMatchList = 1;

	// Spellsearch output

	if (psSearchSpells.Debug)
	{
		WriteChatf("DEBUG :: RecordID: %i szvMatchList: %i", RecordID, szvMatchList);
	}

	for (int x = RecordID - 1; x <= szvMatchList - 1; ++x)
	{
		thisSpell = vTempList.at(x);
		if (thisSpell == nullptr) continue;

		OutputResultConsole(psSearchSpells, thisSpell);
	}
}

void MQ2SpellSearchType::OutputResultConsole(SpellSearch& psSearchSpells, PSPELL& pthisSpell)
{
	psSearchSpells.SpellsShown++;

	std::string strNameColor = "\ag";
	if (!KnowSpell(pthisSpell->ID)) strNameColor = "\ar";

	if (psSearchSpells.ShowDetailedOutput)
	{
		WriteChatf("\at[\aw------------------------------------------------------------------------------\at]");
		WriteChatf("\ayID: \a-y%d \ay[\a-y%s%s\ay] ClassLevel: \a-y%d \n\ayCategory: \a-y%d \aySubcategory: \a-y%d \aySubcategory2: \a-y%d \n\aySpellGroup: \a-y%d \aySpellSubGroup: \a-y%d \n\aySpellClass: \a-y%d \aySpellSubClass: \a-y%d",
			pthisSpell->ID, strNameColor.c_str(), pthisSpell->Name, pthisSpell->ClassLevel[GetPcProfile()->Class],
			pthisSpell->Category, pthisSpell->Subcategory, pthisSpell->Subcategory2,
			pthisSpell->SpellClass, pthisSpell->SpellSubClass,
			pthisSpell->SpellGroup, pthisSpell->SpellSubGroup
		);
	}
	else
	{
		WriteChatf("\aw[ID: %*d] \ao[%*d] %s[%s]", 5, pthisSpell->ID, 3, pthisSpell->ClassLevel[GetPcProfile()->Class], strNameColor.c_str(), pthisSpell->Name);
	}

	if (psSearchSpells.ShowSpellEffects || psSearchSpells.ShowDetailedOutput)
	{
		char szBuff[MAX_STRING] = { 0 };
		char szTemp[MAX_STRING] = { 0 };

		for (int i = 0; i < GetSpellNumEffects(pthisSpell); i++)
		{
			szBuff[0] = szTemp[0] = 0;
			strcat_s(szBuff, ParseSpellEffect(pthisSpell, i, szTemp, sizeof(szTemp)));
			if (szBuff[0] != 0) WriteChatf(" \a-y%s", szBuff);
		}

		DumpPSpellMembers(pthisSpell);
	}
}

/*
	TLO
*/

bool MQ2SpellSearchType::GetSpellSearchState(std::string_view query)
{
	if (!query.size()) return false;

	PcProfile* pProfile = GetPcProfile();

	if (!pProfile) return false;

	// Make a string think its a char...
	const char* szQuery = &query[0];

	SpellSearch psSearchSpells = ParseSpellSearch(szQuery);

	if (psSearchSpells == pSpellSearch->SearchSpells)
	{
		if (pSpellSearch->vMatchesList.size()<1) return false;

		pSpellSearch->SearchSpells.CacheView(psSearchSpells);
	}
	else
	{
		pSpellSearch->SearchSpells.CacheData(psSearchSpells);
		pSpellSearch->SearchSpells.CacheView(psSearchSpells);
		std::vector<PSPELL>& lvMatchesList = FindSpells(pSpellSearch->SearchSpells, false);
		pSpellSearch->vMatchesList = lvMatchesList;
		pSpellSearch->spellsfound = lvMatchesList.size();
	
		if (pSpellSearch->spellsfound < 1) return false;

		pSpellSearch->previousquery = pSpellSearch->query;
		pSpellSearch->query = query;
	}

	int szvMatchList = pSpellSearch->vMatchesList.size();

	// Copy vector over
	std::vector<PSPELL> vTempList;
	for (int i = 0; i < szvMatchList; ++i)
	{
		// Only copy the spells that fit the criteria
		if (OutputFilter(psSearchSpells, pSpellSearch->vMatchesList.at(i))) vTempList.push_back(pSpellSearch->vMatchesList.at(i));
	}

	szvMatchList = vTempList.size();
	pSpellSearch->spellsfound = szvMatchList;

	if (!pSpellSearch->spellsfound) {
		pSpellSearch->Clear();
		return false;
	}

	if (psSearchSpells.ShowReverse)
	{
		sort(vTempList.begin(), vTempList.end(), [](PSPELL a, PSPELL b)
			{
				return (a->ClassLevel[GetPcProfile()->Class] > b->ClassLevel[GetPcProfile()->Class]);
			}
		);
	}

	int recordID = GetVectorRecordID(psSearchSpells, vTempList)-1;
	
	if (psSearchSpells.Debug)
	{
		WriteChatf("recordID %i Records %i", recordID, szvMatchList);
	}
	pSpellSearch->id = vTempList.at(recordID)->ID;
	pSpellSearch->name = vTempList.at(recordID)->Name;
	pSpellSearch->level = vTempList.at(recordID)->ClassLevel[pProfile->Level];
	pSpellSearch->category = vTempList.at(recordID)->Category;
	pSpellSearch->subcategory = vTempList.at(recordID)->Subcategory;
	pSpellSearch->subcategory2 = vTempList.at(recordID)->Subcategory2;
	pSpellSearch->spellclass = vTempList.at(recordID)->SpellClass;
	pSpellSearch->spellsubclass = vTempList.at(recordID)->SpellSubClass;
	pSpellSearch->spellgroupid = vTempList.at(recordID)->SpellGroup;
	pSpellSearch->spellsubgroupid = vTempList.at(recordID)->SpellSubGroup;
	pSpellSearch->recordID = recordID;

	return true;
}

bool MQ2SpellSearchType::dataSpellSearch(const char* szIndex, MQTypeVar& Ret)
{
	pSpellSearch->query = szIndex;

	if (pSpellSearch->query.empty())
	{
		MacroError("[MQ2SpellSearch] Please provide a search query.");
		return false;
	}
	if (pSpellSearch->query.size() > MAX_STRING)
	{
		MacroError("[MQ2SpellSearch] Search query must be no more than %d characters.", MAX_STRING);
		return false;
	}

	Ret.DWord = 0;
	Ret.Type = pSpellSearchType;

	return true;
}

/*
	Plugin API
*/

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("MQ2SpellSearch::Initializing version %f", MQ2Version);

	AddCommand("/spellsearch", SpellSearchCmd);
	AddMQ2Data("SpellSearch", MQ2SpellSearchType::dataSpellSearch);

	pSpellSearchType = new MQ2SpellSearchType;

	WriteChatf("\aw[MQ2SpellSearch] \at/spellsearch \aoby AmericanNero and ChatWithThisName");
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("MQ2SpellSearch::Shutting down");
	RemoveCommand("/spellsearch");

	RemoveMQ2Data("SpellSearch");
	delete pSpellSearchType;
}

/**
 * @fn SetGameState
 *
 * This is called when the GameState changes.  It is also called once after the
 * plugin is initialized.
 *
 * For a list of known GameState values, see the constants that begin with
 * GAMESTATE_.  The most commonly used of these is GAMESTATE_INGAME.
 *
 * When zoning, this is called once after @ref OnBeginZone @ref OnRemoveSpawn
 * and @ref OnRemoveGroundItem are all done and then called once again after
 * @ref OnEndZone and @ref OnAddSpawn are done but prior to @ref OnAddGroundItem
 * and @ref OnZoned
 *
 * @param GameState int - The value of GameState at the time of the call
 */
PLUGIN_API void SetGameState(int GameState)
{
	// DebugSpewAlways("MQ2SpellSearch::SetGameState(%d)", GameState);

	if (GameState == GAMESTATE_INGAME)
	{
		const std::string szAlias = GetPrivateProfileString("Aliases", "/spellsearch", "None", gPathMQini);

		if (szAlias != "None")
		{
			WriteChatf("\awMQ2SpellSearch: \arWarning! The alias /spellsearch already exists. Please delete it by entering \"\ay/alias /spellsearch delete\ar\" then try again.");
			EzCommand("/timed 10 /plugin MQ2SpellSearch unload");
			return;
		}
	}
}

/**
 * @fn OnUpdateImGui
 *
 * This is called each time that the ImGui Overlay is rendered. Use this to render
 * and update plugin specific widgets.
 *
 * Because this happens extremely frequently, it is recommended to move any actual
 * work to a separate call and use this only for updating the display.
 */
/*
PLUGIN_API void OnUpdateImGui()//Maybe for use with an ImGui window to output a list?
{
		if (GetGameState() == GAMESTATE_INGAME)
		{
			if (ShowMQ2SpellSearchWindow)
			{
				if (ImGui::Begin("MQ2SpellSearch", &ShowMQ2SpellSearchWindow, ImGuiWindowFlags_MenuBar))
				{
					if (ImGui::BeginMenuBar())
					{
						ImGui::Text("MQ2SpellSearch is loaded!");
						ImGui::EndMenuBar();
					}
				}
				ImGui::End();
			}
		}
}
*/

// Far from done.
void MQ2SpellSearchType::ShowCMDHelp()
{
	WriteChatf("\n\aw[MQ2SpellSearch] \ayUsage: \at/spellsearch Name Cat SubCat MinLevel MaxLevel ...\n");
	WriteChatf("\n\aw[MQ2SpellSearch] \ayAllows you to search spell information instead of hunting through the spell browser.\n");
	WriteChatf("\n\aw[MQ2SpellSearch] \aySearch Options: Cat \"name\" SubCat \"name\" MinLevel level MaxLevel level");
	WriteChatf("\n\aw[MQ2SpellSearch] \ay                Timer time SPA something ShowMaxRank(flag) Scribable(flag) ShowEffects(flag)\n");
	WriteChatf("\n\aw[MQ2SpellSearch] \ay Use these options for useful search information: List Categories|SPA|SpellMembers\n");
}

void MQ2SpellSearchType::DumpPSpellMembers(PSPELL& pSpell)
{
	WriteChatf("ActorTagID: %i \nAEDuration: %u \nAERange:  %4.2f \nAffectInanimate: %d \nAIValidTargets: %u",
		pSpell->ActorTagId,
		pSpell->AEDuration,
		pSpell->AERange,
		pSpell->AffectInanimate,
		pSpell->AIValidTargets
	);

	WriteChatf("AnimVariation:                 %u \nAutoCast:                      %i \nBaseEffectsFocusCap:           %i \nBaseEffectsFocusOffset:        %i \nBaseEffectsFocusSlope:         %4.2f",
		pSpell->AnimVariation,
		pSpell->Autocast,
		pSpell->BaseEffectsFocusCap,
		pSpell->BaseEffectsFocusOffset,
		pSpell->BaseEffectsFocusSlope
	);

	WriteChatf("[bStacksWithDiscs: %d] \n[BypassRegenCheck: %d] \n[CalcIndex: %i] \n[CanCastInCombat: %d] \n[CanCastOutOfCombat: %d]",
		pSpell->bStacksWithDiscs,
		pSpell->BypassRegenCheck,
		pSpell->CalcIndex,
		pSpell->CanCastInCombat,
		pSpell->CanCastOutOfCombat
	);

	WriteChatf("[CancelOnSit: %u] \n[CanMGB: %d] \n[CannotBeScribed: %d] \n[CastDifficulty: %u] \n[CasterRequirementID: %i]",
		pSpell->CancelOnSit,
		pSpell->CanMGB,
		pSpell->CannotBeScribed,
		pSpell->CastDifficulty,
		pSpell->CasterRequirementID
	);

	WriteChatf("[CastingAnim: %u] \n[CastNotStanding: %d] \n[CastTime: %u]",
		pSpell->CastingAnim,
		pSpell->CastNotStanding,
		pSpell->CastTime
	);

	for (int j = 0; j < sizeof(pSpell->ClassLevel); ++j)
	{
		if (pSpell->ClassLevel[j] > 0 && pSpell->ClassLevel[j] < 255)
		{
			WriteChatf("[ClassLevel: %i - %u]", j, pSpell->ClassLevel[j]);
		}
	}

	WriteChatf("[ConeEndAngle: %i] \n[ConeStartAngle: %i] \n[CountdownHeld: %d] \n[CRC32Marker: %u] \n[CritChanceOverride: %i]",
		pSpell->ConeEndAngle,
		pSpell->ConeStartAngle,
		pSpell->CountdownHeld,
		pSpell->CRC32Marker,
		pSpell->CritChanceOverride
	);

	WriteChatf("[Deletable: %d] \n[DescriptionIndex: %i] \n[Deity: %i] \n[DistanceMod: %4.2f]",
		pSpell->Deletable,
		pSpell->DescriptionIndex,
		pSpell->Deity,
		pSpell->DistanceMod
	);

	WriteChatf("[DurationCap: %u] \n[DurationParticleEffect: %i] \n[DurationType: %u] \n[DurationWindow: %d]",
		pSpell->DurationCap,
		pSpell->DurationParticleEffect,
		pSpell->DurationType,
		pSpell->DurationWindow
	);

	WriteChatf("[EnduranceCost: %i] \n[EnduranceValue: %i] \n[EnduranceUpkeep: %i] \n[Environment: %u]",
		pSpell->EnduranceCost,
		pSpell->EnduranceValue,
		pSpell->EnduranceUpkeep,
		pSpell->Environment
	);

	WriteChatf("[Extra: %s]", pSpell->Extra);

	WriteChatf("[Feedbackable: %d] \n[HateGenerated: %i] \n[HateMod: %i] \n[HitCount: %i] \n[HitCountType: %i]",
		pSpell->Feedbackable,
		pSpell->HateGenerated,
		pSpell->HateMod,
		pSpell->HitCount,
		pSpell->HitCountType
	);

	WriteChatf("[IsSkill: %d] \n[LightType: %u] \n[ManaCost: %i] \n[MaxResist: %i]",
		pSpell->IsSkill,
		pSpell->LightType,
		pSpell->ManaCost,
		pSpell->MaxResist
	);

	WriteChatf("[MaxSpreadTime: %i] \n[MaxTargets: %i] \n[MinRange: %4.2f] \n[MinResist: %i] \n[MinSpreadTime: %i]",
		pSpell->MaxSpreadTime,
		pSpell->MaxTargets,
		pSpell->MinRange,
		pSpell->MinResist,
		pSpell->MinSpreadTime
	);

	WriteChatf("[NoBuffBlock: %d] \n[NoDispell: %d]",
		pSpell->NoBuffBlock,
		pSpell->NoDispell
	);

	for (int j = 0; j < sizeof(pSpell->NoExpendReagent); ++j)
	{
		if (pSpell->NoExpendReagent[j] > 0)
		{
			WriteChatf("[NoExpendReagent: %i - %i]", j, pSpell->NoExpendReagent[j]);
		}
	}

	WriteChatf("[NoHate: %d] \n[NoHealDamageItemMod: %d] \n[NoNPCLOS: %d] \n[NoPartialSave: %d] \n[NoRemove: %d]",
		pSpell->NoHate,
		pSpell->NoHealDamageItemMod,
		pSpell->NoNPCLOS,
		pSpell->NoPartialSave,
		pSpell->NoRemove
	);

	WriteChatf("[NoResist: %d] \n[NoStripOnDeath: %d] \n[NotFocusable: %d] \n[NotStackableDot: %d] \n[NPCChanceofKnowingSpell: %u]",
		pSpell->NoResist,
		pSpell->NoStripOnDeath,
		pSpell->NotFocusable,
		pSpell->NotStackableDot,
		pSpell->NPCChanceofKnowingSpell
	);

	WriteChatf("[NPCMemCategory: %i] \n[NPCUsefulness: %i] \n[NumEffects: %i] \n[OnlyDuringFastRegen: %d] \n[PCNPCOnlyFlag: %i]",
		pSpell->NPCMemCategory,
		pSpell->NPCUsefulness,
		pSpell->NumEffects,
		pSpell->OnlyDuringFastRegen,
		pSpell->PCNPCOnlyFlag
	);

	WriteChatf("[PushBack: %4.2f] \n[PushUp: %4.2f] \n[PVPCalc: %i] \n[PVPDuration: %u] \n[PVPDurationCap: %u]",
		pSpell->PushBack,
		pSpell->PushUp,
		pSpell->PvPCalc,
		pSpell->PvPDuration,
		pSpell->PvPDurationCap
	);

	WriteChatf("[PvPResistBase: %i] \n[PvPResistCap: %i] \n[Range: %4.2f]",
		pSpell->PvPResistBase,
		pSpell->PvPResistCap,
		pSpell->Range
	);

	for (int j = 0; j < sizeof(pSpell->ReagentCount); ++j)
	{
		if (pSpell->ReagentCount[j] > 0)
		{
			WriteChatf("[ReagentCount: %i - %i]", j, pSpell->ReagentCount[j]);
		}
	}

	for (int j = 0; j < sizeof(pSpell->ReagentID); ++j)
	{
		if (pSpell->ReagentID[j] > 0)
		{
			WriteChatf("[ReagentID: %i - %i]", j, pSpell->ReagentID[j]);
		}
	}

	WriteChatf("[RecastTime: %u] \n[RecoveryTime: %u] \n[Reflectable: %d] \n[Resist: %u] \n[ResistAdj: %i]",
		pSpell->RecastTime,
		pSpell->RecoveryTime,
		pSpell->Reflectable,
		pSpell->Resist,
		pSpell->ResistAdj
	);

	WriteChatf("[ResistCap: %i] \n[ResistPerLevel: %i] \n[ReuseTimerIndex: %i] \n[Scribable: %i] \n[ShowWearOffMessage: %d]",
		pSpell->ResistCap,
		pSpell->ResistPerLevel,
		pSpell->ReuseTimerIndex,
		pSpell->Scribable,
		pSpell->ShowWearOffMessage
	);

	WriteChatf("[Skill: %u - %s] \n[SneakAttack: %d] \n[spaindex: %i]",
		pSpell->Skill, szSkills[pSpell->Skill],
		pSpell->SneakAttack,
		pSpell->spaindex
	);

	int nEffects = GetSpellNumEffects(pSpell);

	for (int i = 0; i < nEffects; ++i)
	{
		WriteChatf("Spell Affect (SPA) %i: %i %s", i, GetSpellAttrib(pSpell, i), eEQSPAreversed[GetSpellAttrib(pSpell, i)].c_str());
	}

	WriteChatf("[SpellAnim: %i] \n[SpellIcon: %i] \n[SpellRank: %i] \n[SpellReqAssociationID: %i]",
		pSpell->SpellAnim,
		pSpell->SpellIcon,
		pSpell->SpellRank,
		pSpell->SpellReqAssociationID
	);

	WriteChatf("[SpreadRadius: %i] \n[StacksWithSelf: %d]",
		pSpell->SpreadRadius,
		pSpell->StacksWithSelf
	);

	WriteChatf("[TargetAnim: %u] \n[TargetType: %u] \n[TimeOfDay: %u]",
		pSpell->TargetAnim,
		pSpell->TargetType,
		pSpell->TimeOfDay
	);

	WriteChatf("[TravelType: %u] \n[Uninterruptable: %d] \n[Unknown0x02C: %6.4f] \n[UsesPersistentParticles: %d] \n[ZoneType: %u]",
		pSpell->TravelType,
		pSpell->Uninterruptable,
		pSpell->Unknown0x02C,
		pSpell->UsesPersistentParticles,
		pSpell->ZoneType
	);

}
