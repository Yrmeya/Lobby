[BaseContainerProps()]
class LobbyPlayerData
{
	int m_iPlayerId;
	string m_sPlayerName;
	bool m_bIsReady;
	bool m_bIsGameMaster;
	int m_iSquadIndex;
	int m_iRoleIndex;
	IEntity m_SpectatorEntity;
	IEntity m_CharacterEntity;
	int m_iFetchRetries = 0;
	
	void LobbyPlayerData(int playerId)
	{
		m_iPlayerId     = playerId;
		m_iSquadIndex   = -1;
		m_iRoleIndex    = -1;
		m_bIsReady      = false;
		m_bIsGameMaster = false;
		m_SpectatorEntity = null;
		m_CharacterEntity = null;
		m_sPlayerName   = "";
	}
}
