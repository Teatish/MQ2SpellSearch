// MQ2SpellSearch.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.

#include <mq/Plugin.h>

PreSetup("MQ2SpellSearch");
PLUGIN_VERSION(0.1);

typedef struct SpellSearch {
	int MinLevel;
	int MaxLevel;
	int Category;
	int SubCategory;
	int Timer;
	int SPA;
	bool IgnoreRank;
	bool CanScribe;
} SpellSearch, * pSpellSearch;

bool SpellMatchesSearch(pSpellSearch pSpellSearch, const PSPAWNINFO pChar, const PSPELL pSpell);
void ClearSpellSearch(pSpellSearch pSpellSearch);
char* ParseSpellSearchArgs(char* szArg, char* szRest, pSpellSearch pSpellSearch);
void ParseSpellSearch(const char* Buffer, pSpellSearch pSpellSearch);

/**
 * @fn InitializePlugin
 *
 * This is called once on plugin initialization and can be considered the startup
 * routine for the plugin.
 */
PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("MQ2SpellSearch::Initializing version %f", MQ2Version);
	AddCommand("/spellsearch", SpellSearchCmd);
	//Eventually add a TLO to access this for macros?
	// AddMQ2Data("mytlo", MyTLOData);
}

/**
 * @fn ShutdownPlugin
 *
 * This is called once when the plugin has been asked to shutdown.  The plugin has
 * not actually shut down until this completes.
 */
PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("MQ2SpellSearch::Shutting down");
	RemoveCommand("/spellsearch");
	//Eventually add a TLO to access this for macros?
	// RemoveMQ2Data("mytlo");
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
PLUGIN_API void OnUpdateImGui()//Maybe for use with an ImGui window to output a list?
{
/*
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
*/
}

void SpellSearchCmd(PlayerClient* you, char* szLine) {
	SpellSearch SearchSpells;
	ClearSpellSearch(&SearchSpells);
	/*
	* Example for szLine
	* "MinLevel 100 MaxLevel 115 Category 125 IgnoreRank"
	*/
	ParseSpellSearch(szLine, &SearchSpells);
	std::vector<PSPELL> vMatchList;

	for (EQ_Spell* thisSpell : pSpellMgr->Spells) {
		if (thisSpell == nullptr)
			return;

		if (!SpellMatchesSearch(&SearchSpells, pCharSpawn, thisSpell))//debug statements within function.
			continue;

		vMatchList.push_back(thisSpell);
	}

	if (!vMatchList.empty()) {
		for (EQ_Spell* thisSpell : vMatchList) {
			WriteChatf("Name: \ap%s\ax Level: \at%i\ax.", thisSpell->Name, thisSpell->ClassLevel[GetPcProfile()->Class]);
		}
	}
}

void ClearSpellSearch(pSpellSearch pSpellSearch) {
	if (!pSpellSearch)
		return;

	ZeroMemory(pSpellSearch, sizeof(SpellSearch));

	pSpellSearch->MinLevel = -1;
	pSpellSearch->MaxLevel = MAX_PC_LEVEL + 1;
	pSpellSearch->Category = -1;
	pSpellSearch->SubCategory = -1;
	pSpellSearch->Timer = -1;
	pSpellSearch->SPA = -1;
	pSpellSearch->IgnoreRank = false;
	pSpellSearch->CanScribe = true;
}

char* ParseSpellSearchArgs(char* szArg, char* szRest, pSpellSearch pSpellSearch) {
	if (szArg && pSpellSearch) {
		if (!_stricmp(szArg, "Cat") || (!_stricmp(szArg, "Category"))) {
			GetArg(szArg, szRest, 1);
			pSpellSearch->Category = atoi(szArg);
			szRest = GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "SubCat") || (!_stricmp(szArg, "SubCategory"))) {
			GetArg(szArg, szRest, 1);
			pSpellSearch->SubCategory = atoi(szArg);
			szRest = GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "MinLevel")) {
			GetArg(szArg, szRest, 1);
			pSpellSearch->MinLevel = atoi(szArg);
			szRest = GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "MaxLevel")) {
			GetArg(szArg, szRest, 1);
			pSpellSearch->MaxLevel = atoi(szArg);
			szRest = GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "Timer")) {
			GetArg(szArg, szRest, 1);
			pSpellSearch->Timer = atoi(szArg);
			szRest = GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "SPA")) {
			GetArg(szArg, szRest, 1);
			pSpellSearch->SPA = atoi(szArg);
			WriteChatf("SPA set to %i", pSpellSearch->SPA);
			szRest = GetNextArg(szRest, 1);
		}

		if (!_stricmp(szArg, "IgnoreRank")) {
			pSpellSearch->IgnoreRank = true;
		}

		if (!_stricmp(szArg, "Scribable")) {
			pSpellSearch->CanScribe = !pSpellSearch->CanScribe;
		}
	}
	return szRest;
}

bool SpellMatchesSearch(pSpellSearch pSpellSearch, const PSPAWNINFO pChar, const PSPELL pSpell) {
	//Cat / Category
	if (pSpellSearch->Category != -1) {
		if (pSpell->Category != pSpellSearch->Category)
			return false;
	}

	//SubCat / SubCategory
	if (pSpellSearch->SubCategory != -1) {
		if (pSpell->Subcategory != pSpellSearch->SubCategory) {
			WriteChatf("\ag%s\ax Wrong SubCategory %i != %i.", pSpell->Name, pSpell->Subcategory, pSpellSearch->SubCategory);
			return false;
		}
	}

	//MinLevel
	if (pSpellSearch->MaxLevel != -1 && pSpell->ClassLevel[GetPcProfile()->Class] < pSpellSearch->MinLevel) {
		switch (pSpell->ClassLevel[GetPcProfile()->Class]) {
			case 254:
			case 255:
				break;
			default:
				WriteChatf("\ag%s\ax Below Level %i < %i.", pSpell->Name, pSpell->ClassLevel[GetPcProfile()->Class], pSpellSearch->MinLevel);
				break;
		}
		return false;
	}

	//MaxLevel
	if (pSpellSearch->MaxLevel != MAX_PC_LEVEL + 1 && pSpell->ClassLevel[GetPcProfile()->Class] > pSpellSearch->MaxLevel) {
		switch (pSpell->ClassLevel[GetPcProfile()->Class]) {
			case 254:
			case 255:
				break;
			default:
				WriteChatf("\ag%s\ax Exceeds Level %i > %i.", pSpell->Name, pSpell->ClassLevel[GetPcProfile()->Class], pSpellSearch->MaxLevel);
				break;
		}
		return false;
	}

	//Timer
	if (pSpellSearch->Timer != -1) {
		if (pSpellSearch->Timer != pSpell->ReuseTimerIndex) {
			WriteChatf("\ag%s is the wrong timer", pSpell->Name);
			return false;
		}
	}

	//SPA
	if (pSpellSearch->SPA != -1) {
		bool bFound = false;
		int numeffects = GetSpellNumEffects(pSpell);

		for (int i = 0; i < numeffects; i++) {
			int spafound = GetSpellAttrib(pSpell, i);

			if (spafound == pSpellSearch->SPA) {
				bFound = true;
			}
		}

		if (!bFound) {
			WriteChatf("SPA not found");
			return false;
		}
	}

	if (pSpellSearch->IgnoreRank) {
		if (strstr(pSpell->Name, "Rk. I")) {
			return false;
		}
	}

	if (pSpellSearch->CanScribe && pSpell->CannotBeScribed) {
		WriteChatf("\ag%s\ax Cannot be Scribed", pSpell->Name);
		return false;
	}

	return true;
}

void ParseSpellSearch(const char* Buffer, pSpellSearch pSpellSearch) {
	char szArg[MAX_STRING] = { 0 };
	char szMsg[MAX_STRING] = { 0 };
	char szLLine[MAX_STRING] = { 0 };
	char* szFilter = szLLine;
	bool DidTarget = false;
	bool bArg = true;

	bRunNextCommand = true;
	strcpy_s(szLLine, Buffer);
	_strlwr_s(szLLine);
	while (bArg) {
		GetArg(szArg, szFilter, 1);
		szFilter = GetNextArg(szFilter, 1);
		if (szArg[0] == 0) {
			bArg = false;
		}
		else {
			szFilter = ParseSpellSearchArgs(szArg, szFilter, pSpellSearch);
		}
	}
}

/* //This is just a list of spell members to consider for using for searching spells.
void NothingButAList(PSPELL pSpell) {
	pSpell->ActorTagId;
	pSpell->AEDuration;
	pSpell->AERange;
	pSpell->AffectInanimate;
	pSpell->AIValidTargets;
	pSpell->AnimVariation;
	pSpell->Autocast;
	pSpell->BaseEffectsFocusCap;
	pSpell->BaseEffectsFocusOffset;
	pSpell->BaseEffectsFocusSlope;
	pSpell->bStacksWithDiscs;
	pSpell->BypassRegenCheck;
	pSpell->CalcIndex;
	pSpell->CanCastInCombat;
	pSpell->CanCastOutOfCombat;
	pSpell->CancelOnSit;
	pSpell->CanMGB;
	pSpell->CannotBeScribed;
	pSpell->CastDifficulty;
	pSpell->CasterRequirementID;
	pSpell->CastingAnim;
	pSpell->CastNotStanding;
	pSpell->CastTime;
	pSpell->Category;
	pSpell->ClassLevel;
	pSpell->ConeEndAngle;
	pSpell->ConeStartAngle;
	pSpell->CountdownHeld;
	pSpell->CRC32Marker;
	pSpell->CritChanceOverride;
	pSpell->Deletable;
	pSpell->DescriptionIndex;
	pSpell->Deity;
	pSpell->DistanceMod;
	pSpell->DistanceModEnd;
	pSpell->DistanceModStart;
	pSpell->DurationCap;
	pSpell->DurationParticleEffect;
	pSpell->DurationType;
	pSpell->DurationWindow;
	pSpell->EnduranceCost;
	pSpell->EnduranceValue;
	pSpell->EnduranceUpkeep;
	pSpell->Environment;
	pSpell->Extra;
	pSpell->Feedbackable;
	pSpell->HateGenerated;
	pSpell->HateMod;
	pSpell->HitCount;
	pSpell->HitCountType;
	pSpell->ID;
	pSpell->IsSkill;
	pSpell->LightType;
	pSpell->ManaCost;
	pSpell->MaxResist;
	pSpell->MaxSpreadTime;
	pSpell->MaxTargets;
	pSpell->MinRange;
	pSpell->MinResist;
	pSpell->MinSpreadTime;
	pSpell->Name;
	pSpell->NoBuffBlock;
	pSpell->NoDispell;
	pSpell->NoExpendReagent;
	pSpell->NoHate;
	pSpell->NoHealDamageItemMod;
	pSpell->NoNPCLOS;
	pSpell->NoOverwrite;
	pSpell->NoPartialSave;
	pSpell->NoRemove;
	pSpell->NoResist;
	pSpell->NoStripOnDeath;
	pSpell->NotFocusable;
	pSpell->NotStackableDot;
	pSpell->NPCChanceofKnowingSpell;
	pSpell->NPCMemCategory;
	pSpell->NPCUsefulness;
	pSpell->NumEffects;
	pSpell->OnlyDuringFastRegen;
	pSpell->PCNPCOnlyFlag;
	pSpell->PushBack;
	pSpell->PushUp;
	pSpell->PvPCalc;
	pSpell->PvPDuration;
	pSpell->PvPDurationCap;
	pSpell->PvPResistBase;
	pSpell->PvPResistCap;
	pSpell->Range;
	pSpell->ReagentCount;
	pSpell->ReagentID;
	pSpell->RecastTime;
	pSpell->RecoveryTime;
	pSpell->Reflectable;
	pSpell->Resist;
	pSpell->ResistAdj;
	pSpell->ResistCap;
	pSpell->ResistPerLevel;
	pSpell->ReuseTimerIndex;
	pSpell->Scribable;
	pSpell->ShowWearOffMessage;
	pSpell->Skill;
	pSpell->SneakAttack;
	pSpell->spaindex;
	pSpell->SpellAnim;
	pSpell->SpellClass;
	pSpell->SpellGroup;
	pSpell->SpellIcon;
	pSpell->SpellRank;
	pSpell->SpellRecourseType;
	pSpell->SpellReqAssociationID;
	pSpell->SpellSubClass;
	pSpell->SpellSubGroup;
	pSpell->SpellType;
	pSpell->SpreadRadius;
	pSpell->StacksWithSelf;
	pSpell->Subcategory;
	pSpell->Subcategory2;
	pSpell->TargetAnim;
	pSpell->TargetType;
	pSpell->TimeOfDay;
	pSpell->TravelType;
	pSpell->Uninterruptable;
	pSpell->Unknown0x02C;
	pSpell->UsesPersistentParticles;
	pSpell->ZoneType;
}
*/