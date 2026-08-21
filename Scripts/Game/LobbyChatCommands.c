// Чат-команда #spawnciv: возрождение всех погибших за основную игровую
// фракцию (не CIV). Регистрируется движком как ScrServerCommand (как
// ванильные BanCommand / KickCommand / #login).
class LobbySpawnCivCommand : ScrServerCommand
{
	override string GetKeyword()
	{
		return "spawnciv";
	}

	override bool IsServerSide()
	{
		return true;
	}

	override int RequiredRCONPermission()
	{
		return ERCONPermissions.PERMISSIONS_ADMIN;
	}

	override int RequiredChatPermission()
	{
		// Проверку гейммастера делаем сами на сервере (хост), см. SV_SpawnCiv.
		return EPlayerRole.NONE;
	}

	override ref ScrServerCmdResult OnChatServerExecution(array<string> argv, int playerId)
	{
		LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
		if (rpc)
			rpc.SV_SpawnCiv(playerId);

		return new ScrServerCmdResult("", EServerCmdResultType.OK);
	}

	override ref ScrServerCmdResult OnChatClientExecution(array<string> argv, int playerId)
	{
		return new ScrServerCmdResult("", EServerCmdResultType.OK);
	}

	override ref ScrServerCmdResult OnRCONExecution(array<string> argv)
	{
		return new ScrServerCmdResult("", EServerCmdResultType.OK);
	}

	override ref ScrServerCmdResult OnUpdate()
	{
		return null;
	}
}
