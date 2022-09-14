#include "shim4/main.h"
#include "shim4/shim.h"
#include "shim4/steamworks.h"
#include "shim4/util.h"

using namespace noo;

static std::string steam_language;

namespace noo {

namespace util {

class CSteamOverlayDetect
{
private:
	uint64_t m_iAppID; // Our current AppID
	bool m_bInitialized; // Have we called Request stats and received the callback?

public:
	CSteamOverlayDetect();
	~CSteamOverlayDetect() {}
	
	STEAM_CALLBACK( CSteamOverlayDetect, OnOverlayActivated, GameOverlayActivated_t, 
		m_CallbackOverlayActivated );
};

CSteamOverlayDetect::CSteamOverlayDetect() :
 m_iAppID( 0 ),
 m_bInitialized( false ),
 m_CallbackOverlayActivated( this, &CSteamOverlayDetect::OnOverlayActivated )
{
     m_iAppID = SteamUtils()->GetAppID();
}

class CSteamAchievements
{
private:
	uint64_t m_iAppID; // Our current AppID
	bool m_bInitialized; // Have we called Request stats and received the callback?

public:
	CSteamAchievements();
	~CSteamAchievements() {}
	
	bool RequestStats();
	bool SetAchievement(const char* ID);

	STEAM_CALLBACK( CSteamAchievements, OnUserStatsReceived, UserStatsReceived_t, 
		m_CallbackUserStatsReceived );
	STEAM_CALLBACK( CSteamAchievements, OnUserStatsStored, UserStatsStored_t, 
		m_CallbackUserStatsStored );
	STEAM_CALLBACK( CSteamAchievements, OnAchievementStored, 
		UserAchievementStored_t, m_CallbackAchievementStored );
};

void CSteamOverlayDetect::OnOverlayActivated( GameOverlayActivated_t *pCallback )
{
 // we may get callbacks for other games' stats arriving, ignore them
 if ( pCallback->m_bActive == 1 )
 {
 	if (shim::steam_overlay_activated_callback != nullptr) {
		shim::steam_overlay_activated_callback();
	}
 }
}

static CSteamOverlayDetect *g_SteamOverlayDetect = NULL;

//

CSteamAchievements::CSteamAchievements() :
 m_iAppID( 0 ),
 m_bInitialized( false ),
 m_CallbackUserStatsReceived( this, &CSteamAchievements::OnUserStatsReceived ),
 m_CallbackUserStatsStored( this, &CSteamAchievements::OnUserStatsStored ),
 m_CallbackAchievementStored( this, &CSteamAchievements::OnAchievementStored )
{
     m_iAppID = SteamUtils()->GetAppID();
     RequestStats();
}

bool CSteamAchievements::RequestStats()
{
	// Is Steam loaded? If not we can't get stats.
	if ( NULL == SteamUserStats() || NULL == SteamUser() )
	{
		util::errormsg("RequestStats: Steam not initialised!\n");
		return false;
	}
	// Is the user logged on?  If not we can't get stats.
	if ( !SteamUser()->BLoggedOn() )
	{
		util::errormsg("RequestStats: User not logged in!\n");
		return false;
	}
	// Request user stats.
	return SteamUserStats()->RequestCurrentStats();
}

bool CSteamAchievements::SetAchievement(const char* ID)
{
	// Have we received a call back from Steam yet?
	if (m_bInitialized)
	{
		SteamUserStats()->SetAchievement(ID);
		return SteamUserStats()->StoreStats();
	}
	// If not then we can't set achievements yet
	util::errormsg("Cannot set achievement!\n");
	return false;
}

void CSteamAchievements::OnUserStatsReceived( UserStatsReceived_t *pCallback )
{
 // we may get callbacks for other games' stats arriving, ignore them
 if ( m_iAppID == pCallback->m_nGameID )
 {
   if ( k_EResultOK == pCallback->m_eResult )
   {
     util::debugmsg("Received stats and achievements from Steam\n");
     m_bInitialized = true;
   }
   else
   {
     char buffer[128];
     _snprintf( buffer, 128, "RequestStats - failed, %d\n", pCallback->m_eResult );
     util::errormsg( buffer );
   }
 }
 else {
 	util::errormsg("Got stats for app ID %lld, ours is %lld\n", pCallback->m_nGameID, m_iAppID);
 }
}

void CSteamAchievements::OnUserStatsStored( UserStatsStored_t *pCallback )
{
 // we may get callbacks for other games' stats arriving, ignore them
 if ( m_iAppID == pCallback->m_nGameID )	
 {
   if ( k_EResultOK == pCallback->m_eResult )
   {
     util::debugmsg( "Stored stats for Steam\n" );
   }
   else
   {
     char buffer[128];
     _snprintf( buffer, 128, "StatsStored - failed, %d\n", pCallback->m_eResult );
     util::errormsg( buffer );
   }
 }
}

void CSteamAchievements::OnAchievementStored( UserAchievementStored_t *pCallback )
{
	// we may get callbacks for other games' stats arriving, ignore them
	if (m_iAppID == pCallback->m_nGameID)	{
		util::debugmsg( "Stored Achievement for Steam\n" );
	}
}

static CSteamAchievements *g_SteamAchievements = NULL;

bool achieve_steam(std::string name)
{
	if (shim::steam_init_failed) {
		util::errormsg("steam init failed, not achieving\n");
		return false;
	}
	if (g_SteamAchievements) {
		util::debugmsg(("SetAchievement(" + name + ")\n").c_str());
		static std::string s;
		s = name;
		g_SteamAchievements->SetAchievement(s.c_str());
		return true;
	}
	else {
		util::errormsg("g_SteamAchievements=NULL\n");
		return false;
	}
}

bool start_steamworks()
{
	// Initialize Steam
	bool bRet = SteamAPI_Init();
	// Create the SteamAchievements object if Steam was successfully initialized
	if (bRet) {
		g_SteamAchievements = new CSteamAchievements();
		g_SteamOverlayDetect = new CSteamOverlayDetect();
		steam_language = SteamApps()->GetCurrentGameLanguage();
		if (steam_language == "") {
			steam_language = "english";
		}

#ifdef STEAM_INPUT
		SteamController()->Init();
		SteamController()->RunFrame();
#endif

		// FIXME:
		//SteamUserStats()->ResetAllStats(true);

		return true;
	}
	else {
		util::errormsg("Steam init failed!\n");
		shim::steam_init_failed = true;
		return false;
	}
}

std::string get_steam_language()
{
	return steam_language;
}

} // End namespace util

} // End namespace noo
