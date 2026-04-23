[BaseContainerProps()]
class LobbySquadConfig
{
	[Attribute("", UIWidgets.EditBox, "Squad name (Alpha, Bravo...)")]
	string m_sSquadName;

	[Attribute("", UIWidgets.EditBox, "Squad color hex, e.g. #FF4444")]
	string m_sColor;

	[Attribute(category: "Roles")]
	ref array<ref LobbyRoleConfig> m_aRoles;

	array<ref LobbyRoleConfig> GetRoles()
	{
		if (!m_aRoles)
			m_aRoles = new array<ref LobbyRoleConfig>();
		return m_aRoles;
	}
}
