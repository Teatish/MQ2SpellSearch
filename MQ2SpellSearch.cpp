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
#include <MQ2SpellSearchH.h>

PreSetup("MQ2SpellSearch");
PLUGIN_VERSION(0.1);

using namespace mq::datatypes;

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
		int recordID = -1;
		std::string triggername = "";

		std::string rawquery = "";
		std::string trimmedquery = "";
		std::string query = "";
		std::string previousquery = "";

		SpellSearch SearchSpells;

		std::vector<PSPELL> vMatchesList;
		int spellsfound = 0;
		int spellsshown = 0;

		void Clear()
		{
			id = -1;
			name = "";
			level = -1;
			category = -1;
			subcategory = -1;
			subcategory2 = -1;
			recordID =-1;
			triggername = "";

			rawquery = "";
			trimmedquery = "";
			query = "";
			previousquery = "";

			vMatchesList.clear();
			spellsfound = 0;
			spellsshown = 0;
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
	Count,
	RecordID,
	Query,
	TriggerName,
};

MQ2SpellSearchType::MQ2SpellSearchType() : MQ2Type("SpellSearch")
{
	ScopedTypeMember(SpellSearchMembers, ID);
	ScopedTypeMember(SpellSearchMembers, Name);
	ScopedTypeMember(SpellSearchMembers, Level);
	ScopedTypeMember(SpellSearchMembers, Category);
	ScopedTypeMember(SpellSearchMembers, SubCategory);
	ScopedTypeMember(SpellSearchMembers, SubCategory2);
	ScopedTypeMember(SpellSearchMembers, Count);
	ScopedTypeMember(SpellSearchMembers, RecordID);
	ScopedTypeMember(SpellSearchMembers, Query);
	ScopedTypeMember(SpellSearchMembers, TriggerName);
}

void MQ2SpellSearchType::ShowCMDHelp()
{
	WriteChatf("\n\aw[MQ2SpellSearch] \ayUsage: \at/spellsearch Name Cat SubCat MinLevel MaxLevel ...\n");
	WriteChatf("\n\aw[MQ2SpellSearch] \ayAllows you to search spell information instead of hunting through the spell browser.\n");
	WriteChatf("\n\aw[MQ2SpellSearch] \aySearch Options: Cat \"name\" SubCat \"name\" MinLevel level MaxLevel level");
	WriteChatf("\n\aw[MQ2SpellSearch] \ay                Timer time SPA something ShowMaxRank(flag) Scribable(flag) ShowEffects(flag)\n");
	WriteChatf("\n\aw[MQ2SpellSearch] \ay Use these options for useful search information: List Categories|SPA|SpellMembers\n");
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

	case SpellSearchMembers::TriggerName:
		if (GetSpellSearchState(pSpellSearch->query))
		{
			strcpy_s(DataTypeTemp, pSpellSearch->triggername.c_str());
			Dest.Ptr = &DataTypeTemp[0];
			Dest.Type = pStringType;
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

		// Accept int or the acronym
		if (!_stricmp(szArg, "TargetType") || !_stricmp(szArg, "-targettype"))
		{
			GetArg(szArg, szRest, 1);

			psSpellSearch.TargetType = atoi(szArg);
			if (psSpellSearch.TargetType == 0)
			{
				std::string tmpStr = szArg;
				for (char& c : tmpStr) c = toupper(c);

				auto getTargetType = m_TargetTypeAcronym.find(tmpStr);
				if (getTargetType != m_TargetTypeAcronym.end())
				{
					
					psSpellSearch.TargetType = getTargetType->second;
				}
				else
				{
					// Ignore it if wrong.
					psSpellSearch.TargetType = -1;
				}
			}

			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "Timer") || !_stricmp(szArg, "-timer"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.Timer = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		// Provide a way to submit multiples to tease apart spell lines?
		if (!_stricmp(szArg, "SPA") || !_stricmp(szArg, "-spa"))
		{
			GetArg(szArg, szRest, 1);

			char tmpArg[MAX_STRING] = { 0 };
			strcpy_s(tmpArg, sizeof(tmpArg), szArg);
			_strupr_s(tmpArg);

			// Test for ! (not)
			if (tmpArg[0] == 33)
			{
				psSpellSearch.bSPAMod = false;

				// Delete the leading !
				int i = 0;
				for (i = 1; i < sizeof(tmpArg); ++i)
				{
					tmpArg[i - 1] = tmpArg[i];
				}
				tmpArg[i] = { 0 };
			}

			psSpellSearch.SPA = tmpArg;
			psSpellSearch.nSPA = GetIntFromString(szArg, -1);
			
			if (psSpellSearch.Debug)
			{
				WriteChatf("DEBUG :: ParseSpellSearchArgs :: SPA given %s", tmpArg);
			}
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

		// Used to retrieve the name of spells (Spell: ) from triggers in slots
		// If -triggerindex x is found, then the data retrieved will be from that triggered spell
		if (!_stricmp(szArg, "TriggerIndex") || !_stricmp(szArg, "-triggerindex"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.TriggerIndex = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		// Flag
		if (!_stricmp(szArg, "ShowSpellEffects") || !_stricmp(szArg, "-showspelleffects") || !_stricmp(szArg, "-sse"))
		{
			psSpellSearch.ShowSpellEffects = true;
			return szRest;
		}

		// Search string - Prefix with ! to imply negative match.
		if (!_stricmp(szArg, "SpellEffect") || !_stricmp(szArg, "-spelleffect"))
		{
			GetArg(szArg, szRest, 1);

			char tmpArg[MAX_STRING] = { 0 };
			strcpy_s(tmpArg, sizeof(tmpArg), szArg);
			_strupr_s(tmpArg);

			// Test for ! (not)
			if (tmpArg[0] == 33)
			{
				psSpellSearch.bSpellEffectMod = false;

				// Delete the leading !
				int i = 0;
				for (i = 1; i < sizeof(tmpArg); ++i)
				{
					tmpArg[i - 1] = tmpArg[i];
				}
				tmpArg[i] = { 0 };
			}

			psSpellSearch.SpellEffect = tmpArg;
			return GetNextArg(szRest, 1);
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

		if (!_stricmp(szArg, "NumEffectsMin") || !_stricmp(szArg, "-numeffectsmin"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.NumEffectsMin = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "NumEffectsMax") || !_stricmp(szArg, "-numeffectsmax"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.NumEffectsMax = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "MaxTargetsMin") || !_stricmp(szArg, "-maxtargetsmin"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.MaxTargetsMin = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "MaxTargetsMax") || !_stricmp(szArg, "-maxtargetsmax"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.MaxTargetsMax = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "ResistType") || !_stricmp(szArg, "-resisttype"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.ResistType = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "PushBack") || !_stricmp(szArg, "-pushback"))
		{
			GetArg(szArg, szRest, 1);
			if (!_stricmp(szArg, "yes") || !_stricmp(szArg, "true") || !_stricmp(szArg, "1"))
			{
				psSpellSearch.PushBack = 1;
			}
			else if (!_stricmp(szArg, "no") || !_stricmp(szArg, "false") || !_stricmp(szArg, "0"))
			{
				psSpellSearch.PushBack = 0;
			}
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "PushUp") || !_stricmp(szArg, "-pushup"))
		{
			GetArg(szArg, szRest, 1);
			if (!_stricmp(szArg, "yes") || !_stricmp(szArg, "true") || !_stricmp(szArg, "1"))
			{
				psSpellSearch.PushUp = 1;
			}
			else if (!_stricmp(szArg, "no") || !_stricmp(szArg, "false") || !_stricmp(szArg, "0"))
			{
				psSpellSearch.PushUp = 0;
			}
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "CastTimeMin") || !_stricmp(szArg, "-casttimemin"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.CastTimeMin = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "CastTimeMax") || !_stricmp(szArg, "-casttimemax"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.CastTimeMax = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "RecastTimeMin") || !_stricmp(szArg, "-recasttimemin"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.RecastTimeMin = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "RecastTimeMax") || !_stricmp(szArg, "-recasttimemax"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.RecastTimeMax = atoi(szArg);
			return GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "Extra") || !_stricmp(szArg, "-extra"))
		{
			GetArg(szArg, szRest, 1);
			psSpellSearch.Extra = szArg;
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

	if ((psSpellSearch.NumEffectsMax > -1) && psSpellSearch.NumEffectsMin > psSpellSearch.NumEffectsMax)
	{ 
		int tmp = psSpellSearch.NumEffectsMax;
		psSpellSearch.NumEffectsMax = psSpellSearch.NumEffectsMin;
		psSpellSearch.NumEffectsMin = tmp;
	}

	if ((psSpellSearch.MaxTargetsMax > -1) && psSpellSearch.MaxTargetsMin > psSpellSearch.MaxTargetsMax)
	{
		int tmp = psSpellSearch.MaxTargetsMax;
		psSpellSearch.MaxTargetsMax = psSpellSearch.MaxTargetsMin;
		psSpellSearch.MaxTargetsMin = tmp;
	}

	if (psSpellSearch.ShowFirstRecord && psSpellSearch.ShowLastRecord)
	{
		// They negate, so turn them off. Otherwise the power grid will fail.
		psSpellSearch.ShowFirstRecord = false;
		psSpellSearch.ShowLastRecord = false;
	}

	if ((psSpellSearch.CastTimeMax > -1) && psSpellSearch.CastTimeMin > psSpellSearch.CastTimeMax)
	{
		int tmp = psSpellSearch.CastTimeMax;
		psSpellSearch.CastTimeMax = psSpellSearch.CastTimeMin;
		psSpellSearch.CastTimeMin = tmp;
	}

	if ((psSpellSearch.RecastTimeMax > -1) && psSpellSearch.RecastTimeMin > psSpellSearch.RecastTimeMax)
	{
		int tmp = psSpellSearch.RecastTimeMax;
		psSpellSearch.RecastTimeMax = psSpellSearch.RecastTimeMin;
		psSpellSearch.RecastTimeMin = tmp;
	}

	return psSpellSearch;
}

// If TLO, then WriteChatf is suppressed.
std::vector<PSPELL> MQ2SpellSearchType::FindSpells(SpellSearch& psSearchSpells, bool isCMD)
{
	if (psSearchSpells.Debug)
	{
		WriteChatf("DEBUG :: FindSpells: SpellEffect %s", psSearchSpells.SpellEffect.c_str());
	}

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

		/*
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
		*/
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
	int iSrchLevel = 0;
	int iSrchPartialName = 0;
	int iSrchFeedback = 0;
	int iSrchReflectable = 0;
	int iSrchTargetType = 0;
	int iSrchSkill = 0;
	int iSrchNumEffects = 0;
	int iSrchSpellEffect = 0;
	int iSrchSPA = 0;
	int iSrchMaxTargets = 0;
	int iSrchResistType = 0;
	int iSrchPushBack = 0;
	int iSrchPushUp = 0;
	int iSrchCastTime = 0;
	int iSrchRecastTime = 0;
	int iSrchExtra = 0;

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
		iSrchLevel = 0;
		iSrchPartialName = 0;
		iSrchFeedback = 0;
		iSrchReflectable = 0;
		iSrchTargetType = 0;
		iSrchSkill = 0;
		iSrchNumEffects = 0;
		iSrchSpellEffect = 0;
		iSrchSPA = 0;
		iSrchMaxTargets = 0;
		iSrchResistType = 0;
		iSrchPushBack = 0;
		iSrchPushUp = 0;
		iSrchCastTime = 0;
		iSrchRecastTime = 0;
		iSrchExtra = 0;

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

		if (psSearchSpells.NumEffectsMin != -1 || psSearchSpells.NumEffectsMax != -1)
		{
			iSrchNumEffects = 1;
			NumParams++;

			// If both are specified, we look for the range between. Otherwise, we assume that a match is expected
			if (psSearchSpells.NumEffectsMin > -1 && psSearchSpells.NumEffectsMax > -1)
			{
				if ((int)thisSpell->NumEffects < psSearchSpells.NumEffectsMin || (int)thisSpell->NumEffects > psSearchSpells.NumEffectsMax) continue;
			}
			else if (psSearchSpells.NumEffectsMin > -1)
			{
				if ((int)thisSpell->NumEffects < psSearchSpells.NumEffectsMin) continue;
			}
			else if (psSearchSpells.NumEffectsMax > -1)
			{
				if ((int)thisSpell->NumEffects > psSearchSpells.NumEffectsMax) continue;
			}
		}

		if (psSearchSpells.MaxTargetsMin != -1 || psSearchSpells.MaxTargetsMax != -1)
		{
			iSrchMaxTargets = 1;
			NumParams++;

			// If both are specified, we look for the range between. Otherwise, we assume that a match is expected
			if (psSearchSpells.MaxTargetsMin > -1 && psSearchSpells.MaxTargetsMax > -1)
			{
				if ((int)thisSpell->MaxTargets < psSearchSpells.MaxTargetsMin || (int)thisSpell->MaxTargets > psSearchSpells.MaxTargetsMax) continue;
			}
			else if (psSearchSpells.MaxTargetsMin > -1)
			{
				if ((int)thisSpell->MaxTargets < psSearchSpells.MaxTargetsMin) continue;
			}
			else if (psSearchSpells.MaxTargetsMax > -1)
			{
				if ((int)thisSpell->MaxTargets > psSearchSpells.MaxTargetsMax) continue;
			}
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

		if (psSearchSpells.ResistType != -1)
		{
			iSrchResistType = 1;
			NumParams++;
			if (thisSpell->Resist != psSearchSpells.ResistType) continue;
		}

		if (psSearchSpells.PushBack != -1)
		{
			iSrchPushBack = 1;
			NumParams++;
			if (thisSpell->PushBack == 0 && psSearchSpells.PushBack) continue;
			if (thisSpell->PushBack != 0 && !psSearchSpells.PushBack) continue;
		}

		if (psSearchSpells.PushUp != -1)
		{
			iSrchPushUp = 1;
			NumParams++;
			if (thisSpell->PushUp == 0 && psSearchSpells.PushUp) continue;
			if (thisSpell->PushUp != 0 && !psSearchSpells.PushUp) continue;
		}

		if (psSearchSpells.CastTimeMin != -1 || psSearchSpells.CastTimeMax != -1)
		{
			iSrchCastTime = 1;
			NumParams++;

			if (psSearchSpells.CastTimeMin > -1 && psSearchSpells.CastTimeMax > -1)
			{
				if ((int)thisSpell->CastTime < psSearchSpells.CastTimeMin || (int)thisSpell->CastTime > psSearchSpells.CastTimeMax) continue;
			}
			else if (psSearchSpells.CastTimeMin > -1)
			{
				if ((int)thisSpell->CastTime < psSearchSpells.CastTimeMin) continue;
			}
			else if (psSearchSpells.CastTimeMax > -1)
			{
				if ((int)thisSpell->CastTime > psSearchSpells.CastTimeMax) continue;
			}
		}

		if (psSearchSpells.RecastTimeMin != -1 || psSearchSpells.RecastTimeMax != -1)
		{
			iSrchRecastTime = 1;
			NumParams++;

			if (psSearchSpells.RecastTimeMin > -1 && psSearchSpells.RecastTimeMax > -1)
			{
				if ((int)thisSpell->RecastTime < psSearchSpells.RecastTimeMin || (int)thisSpell->RecastTime > psSearchSpells.RecastTimeMax) continue;
			}
			else if (psSearchSpells.RecastTimeMin > -1)
			{
				if ((int)thisSpell->RecastTime < psSearchSpells.RecastTimeMin) continue;
			}
			else if (psSearchSpells.RecastTimeMax > -1)
			{
				if ((int)thisSpell->RecastTime > psSearchSpells.RecastTimeMax) continue;
			}
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

		// Minimum length of a value of SPA_ is 4 chars
		if (psSearchSpells.nSPA != -1 || psSearchSpells.SPA.length() > 4)
		{
			int nEffects = GetSpellNumEffects(thisSpell);
			bool bFoundSPA = false;

			if (psSearchSpells.nSPA != -1)
			{
				for (int j = 0; j < nEffects; ++j)
				{
					if (GetSpellAttrib(thisSpell, j) == psSearchSpells.nSPA)
					{
						bFoundSPA = true;
						break;
					}
				}
				// Negative match?
				if (bFoundSPA && !psSearchSpells.bSPAMod) continue;
				// Positive match?
				if (!bFoundSPA && psSearchSpells.bSPAMod) continue;
			}
			else
			{
				for (int j = 0; j < nEffects; ++j)
				{
					if (string_equals(eEQSPAreversed[GetSpellAttrib(thisSpell, j)].c_str(), psSearchSpells.SPA.c_str()))
					{
						bFoundSPA = true;
						break;
					}
				}
				// Negative match?
				if (bFoundSPA && !psSearchSpells.bSPAMod) continue;
				// Positive match?
				if (!bFoundSPA && psSearchSpells.bSPAMod) continue;
			}
		}

		// This is an expensive search... hopefully more refined criteria are being applied as well.
		if (!string_equals(psSearchSpells.SpellEffect, ""))
		{
			NumParams++;
			iSrchSpellEffect = 1;

			char szBuff[MAX_STRING] = { 0 };
			char szTemp[MAX_STRING] = { 0 };
			int iPosition = 0;

			for (int i = 0; i < GetSpellNumEffects(thisSpell); i++)
			{
				szBuff[0] = szTemp[0] = 0;
				strcat_s(szBuff, ParseSpellEffect(thisSpell, i, szTemp, sizeof(szTemp)));

				if (szBuff[0] != 0)
				{
					iPosition = ci_find_substr(szBuff, psSearchSpells.SpellEffect.c_str());				
					if (iPosition >= 0)
					{
						break;
					}
				}
			}
			// Negative match?
			if (iPosition >= 0 && !psSearchSpells.bSpellEffectMod) continue;
			// Positive match?
			if (iPosition < 0 && psSearchSpells.bSpellEffectMod) continue;
		}

		if (!string_equals(psSearchSpells.PartialName, ""))
		{
			NumParams++;
			iSrchPartialName = 1;
			int iPosition = ci_find_substr(thisSpell->Name, psSearchSpells.PartialName);
			if (iPosition < 0) continue;
		}

		if (!string_equals(psSearchSpells.Extra, ""))
		{
			NumParams++;
			iSrchExtra = 1;
			int iPosition = ci_find_substr(thisSpell->Extra, psSearchSpells.Extra);
			if (iPosition < 0) continue;
		}

		//bool IsSPAEffect(EQ_Spell* pSpell, int EffectID)
		//int GetSpellAttrib(EQ_Spell* pSpell, int index)
		// 

		// We searched all the parameters we specified. If we are here, we found something.
		if (NumParams == (
							iSrchPartialName +
							iSrchCat +
							iSrchSubCat +
							iSrchSubCat2 +
							iSrchLevel +
							iSrchSkill +
							iSrchFeedback +
							iSrchReflectable +
							iSrchTargetType +
							iSrchNumEffects +
							iSrchSpellEffect +
							iSrchSPA +
							iSrchMaxTargets +
							iSrchResistType +
							iSrchPushBack +
							iSrchPushUp +
							iSrchCastTime +
							iSrchRecastTime +
							iSrchExtra
						))
		{
			pvMatchList.push_back(thisSpell);

			if (psSearchSpells.Debug)
			{
				WriteChatf("DEBUG :: Added [%s] to match list", thisSpell->Name);
			}
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

	// We record the query, but do not make use of it other than debug purposes
	// Subsequent requests may alter view criteria, but the fundamental query
	// remains the same.
	psSearchSpells.RawQuery = szLine;

	if (psSearchSpells.Debug)
	{
		WriteChatf("DEBUG :: Before");
		psSearchSpells.ShowData();
		psSearchSpells.ShowView();
	}

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
		pSpellSearch->Clear();

		pSpellSearch->SearchSpells.CacheData(psSearchSpells);
		pSpellSearch->SearchSpells.CacheView(psSearchSpells);

		std::vector<PSPELL>& lvMatchesList = FindSpells(pSpellSearch->SearchSpells, true);

		pSpellSearch->vMatchesList = lvMatchesList;
		pSpellSearch->spellsfound = lvMatchesList.size();
	}

	if (pSpellSearch->SearchSpells.Debug)
	{
		WriteChatf("DEBUG :: After");
		pSpellSearch->SearchSpells.ShowData();
		pSpellSearch->SearchSpells.ShowView();
	}


	if (pSpellSearch->SearchSpells.Debug)
	{
		WriteChatf("DEBUG :: Spells Found: %i", pSpellSearch->spellsfound);
	}

	if (pSpellSearch->spellsfound > 0) OutputResultsCMD(pSpellSearch->SearchSpells, pSpellSearch->vMatchesList);

	if (pSpellSearch->SearchSpells.Debug)
	{
		WriteChatf("DEBUG :: Spells Shown: %i", pSpellSearch->spellsshown);
	}

	if (!pSpellSearch->spellsshown)
	{
		WriteChatf("\n\aw[MQ2SpellSearch] \ayNo matches found");
	}
	else
	{
		if (pSpellSearch->spellsshown == 1)
		{
			WriteChatf("\n\aw[MQ2SpellSearch] \aoFound %d match", pSpellSearch->spellsshown);
		}
		else
		{
			WriteChatf("\n\aw[MQ2SpellSearch] \aoFound %d matches", pSpellSearch->spellsshown);
		}
	}
}

void MQ2SpellSearchType::OutputResultsCMD(SpellSearch& psSearchSpells, std::vector<PSPELL>& pvMatchList)
{
	PcProfile* pProfile = GetPcProfile();
	if (!pProfile) return;

	PSPELL thisSpell;

	pSpellSearch->spellsshown = 0;
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

	if (!szvMatchList) return;

	if (!psSearchSpells.ShowDetailedOutput) {
		WriteChatf("\n\awID    \aoLVL \atTARGET     \agSPELL                                \ayCATEGORIES");
	}
	else
	{
		WriteChatf("\n\atRAW SPELL DETAILS");
	}

	for (int x = RecordID - 1; x <= szvMatchList - 1; ++x)
	{
		thisSpell = vTempList.at(x);
		if (thisSpell == nullptr) continue;

		OutputResultConsole(psSearchSpells, thisSpell);
	}
	return;
}

void MQ2SpellSearchType::OutputResultConsole(SpellSearch& psSearchSpells, PSPELL& pthisSpell)
{
	pSpellSearch->spellsshown++;

	std::string level = std::to_string(pthisSpell->ClassLevel[GetPcProfile()->Class]);
	if (pthisSpell->ClassLevel[GetPcProfile()->Class] > 253) level = "";

	if (psSearchSpells.ShowDetailedOutput)
	{
		std::string strNameColor = "\ag";
		if (!KnowSpell(pthisSpell->ID)) strNameColor = "\ar!";

		WriteChatf("\ayID                     \aw:\ao %i \n\ayName                   \aw: %s%s \n\ayClassLevel             \aw:\ao %d",
			pthisSpell->ID,
			strNameColor.c_str(),
			pthisSpell->Name,
			pthisSpell->ClassLevel[GetPcProfile()->Class]
		);
		WriteChatf("\ayCategory               \aw:\ao %-*i \aw:\ao \ag%s\ax \n\aySubcategory            \aw:\ao %-*i \aw:\ao \ag%s\ax \n\aySubcategory2           \aw:\ao %-*i \aw:\ao Unsure if useful", 
			3, pthisSpell->Category, CategoryNameLookup[pthisSpell->Category].c_str(),
			3, pthisSpell->Subcategory, CategoryNameLookup[pthisSpell->Subcategory].c_str(),
			3, pthisSpell->Subcategory2
		);
	}
	else
	{
		std::string strNameColor = "\ag ";
		if (!KnowSpell(pthisSpell->ID)) strNameColor = "\ar!";

		WriteChatf("\aw%*d \ao%*s \at%-*s %s%-*s \ay%s -> %s", 5, pthisSpell->ID, 3, level.c_str(), 9, TargetTypeAcronym[pthisSpell->TargetType].c_str(), strNameColor.c_str(), 36, pthisSpell->Name, CategoryNameLookup[pthisSpell->Category].c_str(), CategoryNameLookup[pthisSpell->Subcategory].c_str());
	}

	if (psSearchSpells.ShowSpellEffects || psSearchSpells.ShowDetailedOutput)
	{
		char szBuff[MAX_STRING] = { 0 };
		char szTemp[MAX_STRING] = { 0 };

		for (int i = 0; i < GetSpellNumEffects(pthisSpell); i++)
		{
			szBuff[0] = szTemp[0] = 0;
			strcat_s(szBuff, ParseSpellEffect(pthisSpell, i, szTemp, sizeof(szTemp)));
			if (szBuff[0] != 0) WriteChatf("\aySpellEffect            \aw:\ao %i \aw:\ao %s", i, szBuff);
		}

		int nEffects = GetSpellNumEffects(pthisSpell);
		for (int i = 0; i < nEffects; ++i)
		{
			WriteChatf("\aySpell Affect (SPA)     \aw:\ao %i \aw:\ao %i \aw:\ao %s", i, GetSpellAttrib(pthisSpell, i), eEQSPAreversed[GetSpellAttrib(pthisSpell, i)].c_str());
		}
	}

	if (psSearchSpells.ShowDetailedOutput) DumpPSpellMembers(pthisSpell);

	return;
}

void MQ2SpellSearchType::DumpPSpellMembers(PSPELL& pSpell)
{
	WriteChatf("\ayActorTagID             \aw:\ao %i \n\ayAEDuration             \aw:\ao %u \n\ayAERange                \aw:\ao %4.2f \n\ayAffectInanimate        \aw:\ao %d \n\ayAIValidTargets         \aw:\ao %u",
		pSpell->ActorTagId,
		pSpell->AEDuration,
		pSpell->AERange,
		pSpell->AffectInanimate,
		pSpell->AIValidTargets
	);

	WriteChatf("\ayAnimVariation          \aw:\ao %u \n\ayAutoCast               \aw:\ao %i \n\ayBaseEffectsFocusCap    \aw:\ao %i \n\ayBaseEffectsFocusOffset \aw:\ao %i \n\ayBaseEffectsFocusSlope  \aw:\ao %4.2f",
		pSpell->AnimVariation,
		pSpell->Autocast,
		pSpell->BaseEffectsFocusCap,
		pSpell->BaseEffectsFocusOffset,
		pSpell->BaseEffectsFocusSlope
	);

	WriteChatf("\aybStacksWithDiscs       \aw:\ao %d \n\ayBypassRegenCheck       \aw:\ao %d \n\ayCalcIndex              \aw:\ao %i \n\ayCanCastInCombat        \aw:\ao %d \n\ayCanCastOutOfCombat     \aw:\ao %d",
		pSpell->bStacksWithDiscs,
		pSpell->BypassRegenCheck,
		pSpell->CalcIndex,
		pSpell->CanCastInCombat,
		pSpell->CanCastOutOfCombat
	);

	WriteChatf("\ayCancelOnSit            \aw:\ao %u \n\ayCanMGB                 \aw:\ao %d \n\ayCannotBeScribed        \aw:\ao %d \n\ayCastDifficulty         \aw:\ao %u \n\ayCasterRequirementID    \aw:\ao %i",
		pSpell->CancelOnSit,
		pSpell->CanMGB,
		pSpell->CannotBeScribed,
		pSpell->CastDifficulty,
		pSpell->CasterRequirementID
	);

	WriteChatf("\ayCastingAnim            \aw:\ao %u \n\ayCastNotStanding        \aw:\ao %d \n\ayCastTime               \aw:\ao %u",
		pSpell->CastingAnim,
		pSpell->CastNotStanding,
		pSpell->CastTime
	);

	for (int j = 0; j < sizeof(pSpell->ClassLevel); ++j)
	{
		if (pSpell->ClassLevel[j] > 0 && pSpell->ClassLevel[j] < 255)
		{
			WriteChatf("\ayClassLevel             \aw:\ao %i \aw:\ao %u", j, pSpell->ClassLevel[j]);
		}
	}

	WriteChatf("\ayConeEndAngle           \aw:\ao %i \n\ayConeStartAngle         \aw:\ao %i \n\ayCountdownHeld          \aw:\ao %d \n\ayCRC32Marker            \aw:\ao %u \n\ayCritChanceOverride     \aw:\ao %i",
		pSpell->ConeEndAngle,
		pSpell->ConeStartAngle,
		pSpell->CountdownHeld,
		pSpell->CRC32Marker,
		pSpell->CritChanceOverride
	);

	WriteChatf("\ayDeletable              \aw:\ao %d \n\ayDescriptionIndex       \aw:\ao %i \n\ayDeity                  \aw:\ao %i \n\ayDistanceMod            \aw:\ao %4.2f",
		pSpell->Deletable,
		pSpell->DescriptionIndex,
		pSpell->Deity,
		pSpell->DistanceMod
	);

	WriteChatf("\ayDurationCap            \aw:\ao %u \n\ayDurationParticleEffect \aw:\ao %i \n\ayDurationType           \aw:\ao %u \n\ayDurationWindow         \aw:\ao %d",
		pSpell->DurationCap,
		pSpell->DurationParticleEffect,
		pSpell->DurationType,
		pSpell->DurationWindow
	);

	WriteChatf("\ayEnduranceCost          \aw:\ao %i \n\ayEnduranceValue         \aw:\ao %i \n\ayEnduranceUpkeep        \aw:\ao %i \n\ayEnvironment            \aw:\ao %u",
		pSpell->EnduranceCost,
		pSpell->EnduranceValue,
		pSpell->EnduranceUpkeep,
		pSpell->Environment
	);

	WriteChatf("\ayExtra                  \aw:\ao %s", pSpell->Extra);

	WriteChatf("\ayFeedbackable           \aw:\ao %d \n\ayHateGenerated          \aw:\ao %i \n\ayHateMod                \aw:\ao %i \n\ayHitCount               \aw:\ao %i \n\ayHitCountType           \aw:\ao %i",
		pSpell->Feedbackable,
		pSpell->HateGenerated,
		pSpell->HateMod,
		pSpell->HitCount,
		pSpell->HitCountType
	);

	WriteChatf("\ayIsSkill                \aw:\ao %d \n\ayLightType              \aw:\ao %u \n\ayManaCost               \aw:\ao %i \n\ayMaxResist              \aw:\ao %i",
		pSpell->IsSkill,
		pSpell->LightType,
		pSpell->ManaCost,
		pSpell->MaxResist
	);

	WriteChatf("\ayMaxSpreadTime          \aw:\ao %i \n\ayMaxTargets             \aw:\ao %i \n\ayMinRange               \aw:\ao %4.2f \n\ayMinResist              \aw:\ao %i \n\ayMinSpreadTime          \aw:\ao %i",
		pSpell->MaxSpreadTime,
		pSpell->MaxTargets,
		pSpell->MinRange,
		pSpell->MinResist,
		pSpell->MinSpreadTime
	);

	WriteChatf("\ayNoBuffBlock            \aw:\ao %d \n\ayNoDispell              \aw:\ao %d",
		pSpell->NoBuffBlock,
		pSpell->NoDispell
	);

	for (int j = 0; j < sizeof(pSpell->NoExpendReagent); ++j)
	{
		if (pSpell->NoExpendReagent[j] > 0)
		{
			WriteChatf("\ayNoExpendReagent        \aw:\ao %i \aw:\ao %i", j, pSpell->NoExpendReagent[j]);
		}
	}

	WriteChatf("\ayNoHate                 \aw:\ao %d \n\ayNoHealDamageItemMod    \aw:\ao %d \n\ayNoNPCLOS               \aw:\ao %d \n\ayNoPartialSave          \aw:\ao %d \n\ayNoRemove               \aw:\ao %d",
		pSpell->NoHate,
		pSpell->NoHealDamageItemMod,
		pSpell->NoNPCLOS,
		pSpell->NoPartialSave,
		pSpell->NoRemove
	);

	WriteChatf("\ayNoResist               \aw:\ao %d \n\ayNoStripOnDeath         \aw:\ao %d \n\ayNotFocusable           \aw:\ao %d \n\ayNotStackableDot        \aw:\ao %d \n\ayNPCChanceofKnowingSpell\aw:\ao %u",
		pSpell->NoResist,
		pSpell->NoStripOnDeath,
		pSpell->NotFocusable,
		pSpell->NotStackableDot,
		pSpell->NPCChanceofKnowingSpell
	);

	WriteChatf("\ayNPCMemCategory         \aw:\ao %i \n\ayNPCUsefulness          \aw:\ao %i \n\ayNumEffects             \aw:\ao %i \n\ayOnlyDuringFastRegen    \aw:\ao %d \n\ayPCNPCOnlyFlag          \aw:\ao %i",
		pSpell->NPCMemCategory,
		pSpell->NPCUsefulness,
		pSpell->NumEffects,
		pSpell->OnlyDuringFastRegen,
		pSpell->PCNPCOnlyFlag
	);

	WriteChatf("\ayPushBack               \aw:\ao %4.2f \n\ayPushUp                 \aw:\ao %4.2f \n\ayPVPCalc                \aw:\ao %i \n\ayPVPDuration            \aw:\ao %u \n\ayPVPDurationCap         \aw:\ao %u",
		pSpell->PushBack,
		pSpell->PushUp,
		pSpell->PvPCalc,
		pSpell->PvPDuration,
		pSpell->PvPDurationCap
	);

	WriteChatf("\ayPvPResistBase          \aw:\ao %i \n\ayPvPResistCap           \aw:\ao %i \n\ayRange                  \aw:\ao %4.2f",
		pSpell->PvPResistBase,
		pSpell->PvPResistCap,
		pSpell->Range
	);

	for (int j = 0; j < sizeof(pSpell->ReagentCount); ++j)
	{
		if (pSpell->ReagentCount[j] > 0)
		{
			WriteChatf("\ayReagentCount           \aw:\ao %i \aw:\ao %i", j, pSpell->ReagentCount[j]);
		}
	}

	for (int j = 0; j < sizeof(pSpell->ReagentID); ++j)
	{
		if (pSpell->ReagentID[j] > 0)
		{
			WriteChatf("\ayReagentID              \aw:\ao %i \aw:\ao %i", j, pSpell->ReagentID[j]);
		}
	}

	WriteChatf("\ayRecastTime             \aw:\ao %u \n\ayRecoveryTime           \aw:\ao %u \n\ayReflectable            \aw:\ao %d \n\ayResist                 \aw:\ao %u \n\ayResistAdj              \aw:\ao %i",
		pSpell->RecastTime,
		pSpell->RecoveryTime,
		pSpell->Reflectable,
		pSpell->Resist,
		pSpell->ResistAdj
	);

	WriteChatf("\ayResistCap              \aw:\ao %i \n\ayResistPerLevel         \aw:\ao %i \n\ayReuseTimerIndex        \aw:\ao %i \n\ayScribable              \aw:\ao %i \n\ayShowWearOffMessage     \aw:\ao %d",
		pSpell->ResistCap,
		pSpell->ResistPerLevel,
		pSpell->ReuseTimerIndex,
		pSpell->Scribable,
		pSpell->ShowWearOffMessage
	);

	WriteChatf("\aySkill                  \aw:\ao %u \aw:\ao %s \n\aySneakAttack            \aw:\ao %d \n\ayspaindex               \aw:\ao %i",
		pSpell->Skill, szSkills[pSpell->Skill],
		pSpell->SneakAttack,
		pSpell->spaindex
	);

	// A list linking similar spells at a high level. It doesn't contribute to narrowing down a spell line.
	WriteChatf("\aySpellAnim              \aw:\ao %i \n\aySpellClass             \aw:\ao %i Not useful \n\aySpellSubClass          \aw:\ao %i Not useful \n\aySpellGroup             \aw:\ao %i Not useful \n\aySpellSubGroup          \aw:\ao %i Not useful",
		pSpell->SpellAnim,
		pSpell->SpellClass,
		pSpell->SpellSubClass,
		pSpell->SpellGroup,
		pSpell->SpellSubGroup
	);

	WriteChatf("\aySpellIcon              \aw:\ao %i \n\aySpellRank              \aw:\ao %i \n\aySpellReqAssociationID  \aw:\ao %i",
		pSpell->SpellIcon,
		pSpell->SpellRank,
		pSpell->SpellReqAssociationID
	);

	WriteChatf("\aySpreadRadius           \aw:\ao %i \n\ayStacksWithSelf         \aw:\ao %d",
		pSpell->SpreadRadius,
		pSpell->StacksWithSelf
	);

	WriteChatf("\ayTargetAnim             \aw:\ao %u \n\ayTargetType             \aw:\ao %u \aw:\ao %s \n\ayTimeOfDay              \aw:\ao %u",
		pSpell->TargetAnim,
		pSpell->TargetType, TargetTypeAcronym[pSpell->TargetType].c_str(),
		pSpell->TimeOfDay
	);

	WriteChatf("\ayTravelType             \aw:\ao %u \n\ayUninterruptable        \aw:\ao %d \n\ayUnknown0x02C           \aw:\ao %6.4f \n\ayUsesPersistentParticles\aw:\ao %d \n\ayZoneType               \aw:\ao %u",
		pSpell->TravelType,
		pSpell->Uninterruptable,
		pSpell->Unknown0x02C,
		pSpell->UsesPersistentParticles,
		pSpell->ZoneType
	);

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
		if (pSpellSearch->vMatchesList.size() < 1) return false;

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

	int recordID = GetVectorRecordID(psSearchSpells, vTempList) - 1;

	if (psSearchSpells.Debug)
	{
		WriteChatf("recordID %i Records %i", recordID, szvMatchList);
	}
	pSpellSearch->id = vTempList.at(recordID)->ID;

	if (pSpellSearch->SearchSpells.IgnoreRank)
	{
		const char* szTmp;
		szTmp = &vTempList.at(recordID)->Name[0];
		char szTmp2[MAX_STRING] = { 0 };
		strcpy_s(szTmp2, sizeof(szTmp2), szTmp);

		TruncateSpellRankName(szTmp2);
		pSpellSearch->name = szTmp2;
	}
	else
	{
		pSpellSearch->name = vTempList.at(recordID)->Name;
	}

	pSpellSearch->level = vTempList.at(recordID)->ClassLevel[pProfile->Level];
	pSpellSearch->category = vTempList.at(recordID)->Category;
	pSpellSearch->subcategory = vTempList.at(recordID)->Subcategory;
	pSpellSearch->subcategory2 = vTempList.at(recordID)->Subcategory2;
	pSpellSearch->recordID = recordID;

	if (pSpellSearch->SearchSpells.TriggerIndex != -1)
	{
		int triggerindex = pSpellSearch->SearchSpells.TriggerIndex;
		int numeffects = GetSpellNumEffects(vTempList.at(recordID));

		if (triggerindex >= numeffects) return false;

		char szBuff[MAX_STRING] = { 0 };
		char szTemp[MAX_STRING] = { 0 };

		szBuff[0] = szTemp[0] = 0;
		std::string_view tmpStr;

		strcat_s(szBuff, ParseSpellEffect(vTempList.at(recordID), triggerindex, szTemp, sizeof(szTemp)));
		if (szBuff[0] == 0) return false;

		tmpStr = szBuff;

		if (ci_find_substr(tmpStr, "Spell: ") == -1) return false;

		// Can be 2 of these in there...
		tmpStr.remove_prefix(ci_find_substr(tmpStr, "Spell: ")+7);
		tmpStr.remove_prefix(ci_find_substr(tmpStr, "Spell: ") + 7);

		int closingposition = ci_find_substr(tmpStr, ")");
		if (closingposition != -1)
		{
			int suffixindice = tmpStr.length() - closingposition;
			tmpStr.remove_suffix(suffixindice);
		}

		if (pSpellSearch->SearchSpells.IgnoreRank)
		{
			const char* szTmp;
			szTmp = &tmpStr[0];
			char szTmp2[MAX_STRING] = { 0 };
			strcpy_s(szTmp2, sizeof(szTmp2), szTmp);

			TruncateSpellRankName(szTmp2);
			pSpellSearch->triggername = szTmp2;
		}
		else
		{
			pSpellSearch->triggername = tmpStr;
		}
	}
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
