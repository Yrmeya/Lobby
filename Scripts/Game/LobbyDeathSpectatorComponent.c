[ComponentEditorProps(category: "Lobby", description: "Handles player death by moving them to CIV space spectator")]
class LobbyDeathSpectatorComponentClass : SCR_BaseGameModeComponentClass {}

class LobbyDeathSpectatorComponent : SCR_BaseGameModeComponent
{
    [Attribute("", UIWidgets.ResourcePickerThumbnail, "Spectator prefab (same as in LobbyManager)", "et", category: "LobbyDeathSpectator")]
    protected ResourceName m_sSpectatorPrefab;

    protected ref set<int> m_aDeadPlayers = new set<int>();
    protected ref map<int, IEntity> m_mSpaceCivs = new map<int, IEntity>();

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        if (!Replication.IsServer()) return;
        
        Print("[DeathSpectator] Component initialized. Starting death detection timer.", LogLevel.NORMAL);
        
        // Запускаем таймер, который будет срабатывать каждые 500 мс (2 раза в секунду)
        // Это надежнее, чем EOnFrame, который может не вызываться на GameMode
        GetGame().GetCallqueue().CallLater(DeathDetectionTick, 500, true);
    }

    protected void DeathDetectionTick()
    {
        if (!Replication.IsServer()) return;
        
        LobbyManagerComponent lobbyMgr = LobbyManagerComponent.GetInstance();
        if (!lobbyMgr) return;

        // Если лобби еще идет - ничего не делаем
        if (lobbyMgr.IsLobbyActive()) return;
        
        PlayerManager pm = GetGame().GetPlayerManager();
        if (!pm) return;

        array<int> playerIds = {};
        pm.GetPlayers(playerIds);
        
        if (playerIds.IsEmpty()) return;

        foreach (int pid : playerIds)
        {
            if (m_aDeadPlayers.Contains(pid)) continue;

            PlayerController pc = pm.GetPlayerController(pid);
            if (!pc) continue;

            IEntity controlledEnt = pc.GetControlledEntity();
            if (!controlledEnt) continue;

            bool isDead = false;
            SCR_DamageManagerComponent dmgMgr = SCR_DamageManagerComponent.Cast(controlledEnt.FindComponent(SCR_DamageManagerComponent));
            if (dmgMgr)
            {
                if (dmgMgr.GetHealth() <= 0.0)
                    isDead = true;
            }
            else
            {
                continue;
            }

            if (isDead)
            {
                Print(string.Format("[DeathSpectator] Player %1 IS DEAD! Triggering HandlePlayerDeath.", pid), LogLevel.WARNING);
                m_aDeadPlayers.Insert(pid);
                HandlePlayerDeath(pid);
            }
        }
    }

    protected void HandlePlayerDeath(int playerId)
    {
        Print(string.Format("[DeathSpectator] HandlePlayerDeath START for %1", playerId), LogLevel.NORMAL);
        
        GetGame().GetCallqueue().CallLater(SpawnCivInSpace, 3000, false, playerId);
        GetGame().GetCallqueue().CallLater(MoveCameraToGround, 3000, false, playerId);
    }

    protected void SpawnCivInSpace(int playerId)
    {
        Print(string.Format("[DeathSpectator] SpawnCivInSpace START for %1", playerId), LogLevel.NORMAL);
        
        ResourceName prefab = m_sSpectatorPrefab;
        if (prefab == "")
        {
            Print("[DeathSpectator] ERROR: SpectatorPrefab is EMPTY! Check component settings.", LogLevel.ERROR);
            return;
        }

        Resource res = Resource.Load(prefab);
        if (!res || !res.IsValid())
        {
            Print("[DeathSpectator] ERROR: SpectatorPrefab resource is invalid!", LogLevel.ERROR);
            return;
        }

        vector spawnPos = "0 100000 0"; // Космос

        vector spawnMat[4];
        Math3D.MatrixIdentity4(spawnMat);
        spawnMat[3] = spawnPos;

        EntitySpawnParams params = new EntitySpawnParams();
        params.TransformMode = ETransformMode.WORLD;
        params.Transform = spawnMat;

        IEntity entity = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
        if (!entity)
        {
            Print("[DeathSpectator] ERROR: Failed to spawn CIV entity in space!", LogLevel.ERROR);
            return;
        }
        
        Print(string.Format("[DeathSpectator] CIV entity spawned successfully. Type: %1", entity.Type()), LogLevel.NORMAL);

        m_mSpaceCivs.Set(playerId, entity);

        PlayerController pcSpec = GetGame().GetPlayerManager().GetPlayerController(playerId);
        if (pcSpec)
        {
            RplComponent rpl = RplComponent.Cast(entity.FindComponent(RplComponent));
            if (rpl) rpl.GiveExt(pcSpec.GetRplIdentity(), false);
            
            // Перехватываем управление с мертвого тела на CIV в космосе
            pcSpec.SetControlledEntity(entity);
            Print(string.Format("[DeathSpectator] Control given to CIV space entity for player %1", playerId), LogLevel.NORMAL);
        }
        else
        {
            Print("[DeathSpectator] ERROR: PlayerController lost before giving control!", LogLevel.ERROR);
        }

        ChimeraCharacter character = ChimeraCharacter.Cast(entity);
        if (character)
        {
            entity.ClearFlags(EntityFlags.VISIBLE);
            
            // Устанавливаем фракцию CIV
            SCR_FactionManager facMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
            if (facMgr)
            {
                Faction civFaction = facMgr.GetFactionByKey("CIV");
                if (civFaction)
                {
                    FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
                    if (facComp) facComp.SetAffiliatedFaction(civFaction);
                    
                    SCR_PlayerFactionAffiliationComponent playerFacComp = SCR_PlayerFactionAffiliationComponent.Cast(pcSpec.FindComponent(SCR_PlayerFactionAffiliationComponent));
                    if (playerFacComp)
                    {
                        playerFacComp.SetAffiliatedFaction(civFaction);
                        facMgr.UpdatePlayerFaction_S(playerFacComp);
                    }
                }
                else
                {
                    Print("[DeathSpectator] WARNING: Faction 'CIV' not found!", LogLevel.WARNING);
                }
            }

            SCR_DamageManagerComponent dmgMgr = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
            if (dmgMgr) dmgMgr.FullHeal();
            
            Physics phys = entity.GetPhysics();
            if (phys)
            {
                phys.EnableGravity(false);
                phys.SetVelocity("0 0 0");
                phys.SetAngularVelocity("0 0 0");
            }
            
            SCR_CharacterControllerComponent charCtrl = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
            if (charCtrl)
            {
                charCtrl.SetDisableMovementControls(true); 
                charCtrl.SetDisableWeaponControls(true);   
            }
        }
    }

    protected void MoveCameraToGround(int playerId)
    {
        Print(string.Format("[DeathSpectator] MoveCameraToGround START for %1", playerId), LogLevel.NORMAL);
        
        if (!Replication.IsServer()) return;
        
        vector groundPos = vector.Zero;
        array<SCR_SpawnPoint> spawnPoints = SCR_SpawnPoint.GetSpawnPoints();
        if (spawnPoints && !spawnPoints.IsEmpty())
            groundPos = spawnPoints[0].GetOrigin();
        else
            Print("[DeathSpectator] WARNING: No spawn points found on map!", LogLevel.WARNING);
            
        groundPos[1] = groundPos[1] + 50.0; // Поднимаем камеру на 50 метров

        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc) 
        {
            rpc.BroadcastJIPCameraSpawn(playerId, groundPos);
            Print(string.Format("[DeathSpectator] BroadcastJIPCameraSpawn sent. Pos: %1", groundPos), LogLevel.NORMAL);
        }
        else
        {
            Print("[DeathSpectator] ERROR: LobbyRPCComponent not found!", LogLevel.ERROR);
        }
    }

    //------------------------------------------------------------------------------------------------
    //! Возвращает копию списка погибших (основной источник правды для #spawnciv).
    array<int> GetDeadPlayerIds()
    {
        array<int> result = {};
        foreach (int pid : m_aDeadPlayers)
            result.Insert(pid);
        return result;
    }

    //------------------------------------------------------------------------------------------------
    //! Вызывается после возрождения игрока (#spawnciv): убираем его из списка
    //! мёртвых и удаляем CIV-болванчик в космосе, чтобы не копились сущности
    //! и следующая смерть игрока снова отслеживалась.
    void NotifyPlayerRespawned(int playerId)
    {
        m_aDeadPlayers.Remove(playerId);

        IEntity spaceCiv = m_mSpaceCivs.Get(playerId);
        if (spaceCiv)
        {
            SCR_EntityHelper.DeleteEntityAndChildren(spaceCiv);
            m_mSpaceCivs.Remove(playerId);
        }
    }

    override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
    {
        super.OnPlayerDisconnected(playerId, cause, timeout);
        if (!Replication.IsServer()) return;
        
        m_aDeadPlayers.Remove(playerId); 
        
        IEntity spaceCiv = m_mSpaceCivs.Get(playerId);
        if (spaceCiv)
        {
            SCR_EntityHelper.DeleteEntityAndChildren(spaceCiv);
            m_mSpaceCivs.Remove(playerId);
        }
    }

    static LobbyDeathSpectatorComponent GetInstance()
    {
        SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (!gameMode) return null;
        return LobbyDeathSpectatorComponent.Cast(gameMode.FindComponent(LobbyDeathSpectatorComponent));
    }
}