[BaseContainerProps()]
class LobbyRoleConfig
{
	[Attribute("", UIWidgets.EditBox, "Role name (displayed in UI)")]
	string m_sRoleName;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Character prefab", "et")]
	ResourceName m_sCharacterPrefab;

	[Attribute("", UIWidgets.EditBox, "Faction key (US, USSR, FIA...)")]
	string m_sFactionKey;

	[Attribute("1", UIWidgets.EditBox, "Max players for this role")]
	int m_iMaxPlayers;

	[Attribute("", UIWidgets.EditBox, "Role description (optional)")]
	string m_sDescription;
}
