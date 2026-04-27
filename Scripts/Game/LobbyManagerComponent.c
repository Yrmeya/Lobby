[ComponentEditorProps(category: "Lobby", description: "PvE Lobby Manager")]
class LobbyManagerComponentClass : SCR_BaseGameModeComponentClass {}

class LobbyManagerComponent : SCR_BaseGameModeComponent
{
    [Attribute(category: "Lobby: Squads")]
    protected ref array<ref LobbySquadConfig> m_aSquads;

    [Attribute("", UIWidgets.ResourcePickerThumbnail, "Spectator prefab", "et", category: "LobbyManagerComponent")]
    protected ResourceName m_sSpectatorPrefab;

    [Attribute("1.5", UIWidgets.EditBox, "Spawn random offset radius (m)", category: "LobbyManagerComponent: Settings")]
    protected float m_fSpawnOffsetRadius;

    [Attribute("", UIWidgets.EditBox, "Player Faction Key (e.g. US, USSR)", category: "LobbyManagerComponent: Settings")]
    protected string m_sPlayerFactionKey;

    [RplProp(onRplName: "OnLobbyActiveChange")]
    protected bool m_bLobbyActive = true;

    protected ref map<int, ref LobbyPlayerData> m_mPlayers    = new map<int, ref LobbyPlayerData>();
    protected ref map<int, IEntity>              m_mSpectators = new map<int, IEntity>();
    protected ref set<int>                       m_aDeadPlayers = new set<int>();
    
    protected ref array<int> m_aCreatedGroupIDs = {};
    
    protected bool m_bLocalJIPSpectator = false;

    array<ref LobbySquadConfig> GetSquads()           { return m_aSquads; }
    map<int, ref LobbyPlayerData> GetPlayersMap()     { return m_mPlayers; }
    LobbyPlayerData GetPlayerData(int playerId)       { return m_mPlayers.Get(playerId); }
    int GetPlayerCount()                              { return m_mPlayers.Count(); }
    bool IsLobbyActive()                              { return m_bLobbyActive; }
    
    bool IsLocalPlayerJIPSpectator()                  { return m_bLocalJIPSpectator; }
    void SetLocalJIPSpectator(bool state)             { m_bLocalJIPSpectator = state; }

    array<ref LobbyRoleConfig> GetSquadRoles(int squadIndex)
    {
        if (squadIndex < 0 || squadIndex >= m_aSquads.Count()) return null;
        return m_aSquads[squadIndex].GetRoles();
    }

    string GetSquadName(int squadIndex)
    {
        if (squadIndex < 0 || squadIndex >= m_aSquads.Count()) return "---";
        return m_aSquads[squadIndex].m_sSquadName;
    }

    string GetRoleName(int squadIndex, int roleIndex)
    {
        array<ref LobbyRoleConfig> roles = GetSquadRoles(squadIndex);
        if (!roles || roleIndex < 0 || roleIndex >= roles.Count()) return "---";
        return roles[roleIndex].m_sRoleName;
    }

    int GetReadyCount()
    {
        int count = 0;
        foreach (int id, LobbyPlayerData data : m_mPlayers)
            if (data.m_bIsReady) count++;
        return count;
    }

    int GetRolePlayerCount(int squadIndex, int roleIndex)
    {
        int count = 0;
        foreach (int id, LobbyPlayerData data : m_mPlayers)
            if (data.m_iSquadIndex == squadIndex && data.m_iRoleIndex == roleIndex)
                count++;
        return count;
    }

    void AssignRole_S(int playerId, int squadIndex, int roleIndex)
    {
        if (!Replication.IsServer()) return;
        LobbyPlayerData data = m_mPlayers.Get(playerId);
        if (!data) return;

        if (squadIndex < 0 || roleIndex < 0)
        {
            data.m_iSquadIndex = -1;
            data.m_iRoleIndex  = -1;
            LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
            if (rpc) rpc.BroadcastPlayerAssignment(playerId, -1, -1);
            return;
        }

        if (squadIndex >= m_aSquads.Count()) return;
        array<ref LobbyRoleConfig> roles = m_aSquads[squadIndex].GetRoles();
        if (!roles || roleIndex >= roles.Count()) return;

        int occupied = 0;
        foreach (int id, LobbyPlayerData pd : m_mPlayers)
            if (id != playerId && pd.m_iSquadIndex == squadIndex && pd.m_iRoleIndex == roleIndex)
                occupied++;

        if (occupied >= roles[roleIndex].m_iMaxPlayers) return;

        data.m_iSquadIndex = squadIndex;
        data.m_iRoleIndex  = roleIndex;

        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc) rpc.BroadcastPlayerAssignment(playerId, squadIndex, roleIndex);
    }

    protected void SyncRosterToAll_S()
    {
        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (!rpc) return;
        foreach (int id, LobbyPlayerData data : m_mPlayers)
        {
            rpc.BroadcastPlayerJoined(id, data.m_sPlayerName);
            rpc.BroadcastPlayerAssignment(id, data.m_iSquadIndex, data.m_iRoleIndex);
        }
        rpc.BroadcastHeaderUpdate();
    }

    protected void FetchPlayerName(int playerId)
    {
        LobbyPlayerData data = m_mPlayers.Get(playerId);
        if (!data) return;
        PlayerManager pm = GetGame().GetPlayerManager();
        string name = "";
        if (pm) name = pm.GetPlayerName(playerId);

        if (name == "")
        {
            data.m_iFetchRetries = data.m_iFetchRetries + 1;
            if (data.m_iFetchRetries < 10)
            {
                GetGame().GetCallqueue().CallLater(FetchPlayerName, 500, false, playerId);
                return;
            }
            name = "Player " + playerId;
        }
        data.m_sPlayerName = name;
        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc)
        {
            rpc.BroadcastPlayerJoined(playerId, data.m_sPlayerName);
            rpc.BroadcastHeaderUpdate();
        }
        GetGame().GetCallqueue().CallLater(SyncRosterToAll_S, 300, false);
    }

    void RequestStartGame()
    {
        if (!Replication.IsServer()) return;
        if (!m_bLobbyActive) return;
        
        SetLobbyActive(false);
        GetGame().GetCallqueue().CallLater(SpawnAllAndClose, 100, false);
    }

    protected void SetLobbyActive(bool state)
    {
        if (m_bLobbyActive == state) return;
        m_bLobbyActive = state;
        Replication.BumpMe();
    }

    protected void OnLobbyActiveChange()
    {
        Print("[LobbyManagerComponent] Lobby active state changed to: " + m_bLobbyActive);
    }

    protected void SpawnAllAndClose()
    {
        m_aCreatedGroupIDs.Clear();
        
        foreach (int pid, LobbyPlayerData data : m_mPlayers)
            SpawnPlayerWithRole(pid, data);

        GetGame().GetCallqueue().CallLater(AssignCharactersAndClose, 1000, false);
    }

    protected void AssignCharactersAndClose()
    {
        SCR_GroupsManagerComponent groupsMgr = SCR_GroupsManagerComponent.GetInstance();
        Faction playerFaction = null;
        
        if (groupsMgr && m_sPlayerFactionKey != "")
        {
            FactionManager facMgr = GetGame().GetFactionManager();
            if (facMgr)
                playerFaction = facMgr.GetFactionByKey(m_sPlayerFactionKey);
        }

        if (playerFaction)
        {
            for (int i = 0; i < m_aSquads.Count(); i++)
            {
                SCR_AIGroup newGroup = groupsMgr.CreateNewPlayableGroup(playerFaction);
                if (newGroup)
                {
                    int groupID = newGroup.GetGroupID();
                    m_aCreatedGroupIDs.Insert(groupID);
                    Print(string.Format("[LobbyMgr] Created radio group %1 for squad %2", groupID, i), LogLevel.NORMAL);
                }
                else
                {
                    m_aCreatedGroupIDs.Insert(-1); 
                    Print("[LobbyMgr] ERROR: Failed to create group!", LogLevel.ERROR);
                }
            }
        }
        else
        {
            Print("[LobbyMgr] ERROR: Invalid faction key in LobbyManagerComponent settings!", LogLevel.ERROR);
        }

        foreach (int pid, LobbyPlayerData data : m_mPlayers)
        {
            if (!data.m_CharacterEntity) continue;
            
            PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(pid);
            if (!pc) continue;

            AIControlComponent aiControl = AIControlComponent.Cast(data.m_CharacterEntity.FindComponent(AIControlComponent));
            if (aiControl)
                aiControl.DeactivateAI();

            RplComponent rpl = RplComponent.Cast(data.m_CharacterEntity.FindComponent(RplComponent));
            if (rpl)
                rpl.GiveExt(pc.GetRplIdentity(), false);

            pc.SetControlledEntity(data.m_CharacterEntity);

            if (groupsMgr && data.m_iSquadIndex >= 0 && data.m_iSquadIndex < m_aCreatedGroupIDs.Count())
            {
                int targetGroupID = m_aCreatedGroupIDs[data.m_iSquadIndex];
                if (targetGroupID != -1)
                {
                    int result = groupsMgr.AddPlayerToGroup(targetGroupID, pid);
                    if (result != -1)
                        Print("[LobbyMgr] Player " + pid + " assigned to radio group " + targetGroupID, LogLevel.NORMAL);
                }
            }
        }
        
        GetGame().GetCallqueue().CallLater(BroadcastClose_Delayed, 1000, false);
    }

    protected void BroadcastClose_Delayed()
    {
        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc) rpc.BroadcastLobbyClose();
        GetGame().GetCallqueue().CallLater(CleanupSpectators_Delayed, 2000, false);
    }

    protected void CleanupSpectators_Delayed()
    {
        foreach (int id, IEntity spectator : m_mSpectators)
        {
            if (!spectator) continue;
            PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(id);
            if (pc && pc.GetControlledEntity() == spectator)
            {
                LobbyPlayerData data = m_mPlayers.Get(id);
                if (data && data.m_CharacterEntity) pc.SetControlledEntity(data.m_CharacterEntity);
            }
            SCR_EntityHelper.DeleteEntityAndChildren(spectator);
        }
        m_mSpectators.Clear();
    }

    protected bool SpawnPlayerWithRole(int playerId, LobbyPlayerData data)
    {
        ResourceName prefab;
        if (data.m_iSquadIndex >= 0 && data.m_iRoleIndex >= 0)
        {
            array<ref LobbyRoleConfig> roles = GetSquadRoles(data.m_iSquadIndex);
            if (roles && data.m_iRoleIndex < roles.Count())
                prefab = roles[data.m_iRoleIndex].m_sCharacterPrefab;
        }
        
        if (prefab == "") return false;

        Resource res = Resource.Load(prefab);
        if (!res || !res.IsValid()) return false;

        vector spawnPos = GetSpawnPosition(data.m_iSquadIndex);
        vector spawnMat[4];
        Math3D.MatrixIdentity4(spawnMat);
        spawnMat[3] = spawnPos;

        EntitySpawnParams params = new EntitySpawnParams();
        params.TransformMode = ETransformMode.WORLD;
        params.Transform = spawnMat;

        IEntity character = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
        if (!character) return false;

        data.m_CharacterEntity = character; 
        return true;
    }

    protected vector GetSpawnPosition(int squadIndex)
    {
        array<SCR_SpawnPoint> spawnPoints = SCR_SpawnPoint.GetSpawnPoints();
        vector pos = vector.Zero;
        if (spawnPoints && !spawnPoints.IsEmpty())
        {
            int idx = squadIndex;
            if (idx < 0) idx = 0;
            if (idx >= spawnPoints.Count()) idx = spawnPoints.Count() - 1;
            pos = spawnPoints[idx].GetOrigin();
        }
        float angle = Math.RandomFloat(0, Math.PI2);
        float dist  = Math.RandomFloat(0, m_fSpawnOffsetRadius);
        pos = pos + Vector(Math.Cos(angle) * dist, 0, Math.Sin(angle) * dist);
        return pos;
    }

    protected void SpawnSpectatorForPlayer(int playerId)
    {
        ResourceName prefab = m_sSpectatorPrefab;
        if (prefab == "") 
        {
            Print("[LobbyMgr] SpectatorPrefab is empty!", LogLevel.ERROR);
            return;
        }

        Resource res = Resource.Load(prefab);
        if (!res || !res.IsValid()) return;

        array<SCR_SpawnPoint> spawnPoints = SCR_SpawnPoint.GetSpawnPoints();
        vector spawnPos = vector.Zero;
        if (spawnPoints && !spawnPoints.IsEmpty())
            spawnPos = spawnPoints[0].GetOrigin();

        vector spawnMat[4];
        Math3D.MatrixIdentity4(spawnMat);
        spawnMat[3] = spawnPos;

        EntitySpawnParams params = new EntitySpawnParams();
        params.TransformMode = ETransformMode.WORLD;
        params.Transform = spawnMat;

        IEntity entity = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
        if (!entity) return;

        m_mSpectators.Set(playerId, entity);
        LobbyPlayerData pdata = m_mPlayers.Get(playerId);
        if (pdata) pdata.m_SpectatorEntity = entity;

        PlayerController pcSpec = GetGame().GetPlayerManager().GetPlayerController(playerId);
        if (pcSpec)
        {
            // МАГИЯ СЕТЕВОГО ВЛАДЕНИЯ: Без этого сервер не будет маршрутизировать голос от этой болванки!
            RplComponent rpl = RplComponent.Cast(entity.FindComponent(RplComponent));
            if (rpl)
                rpl.GiveExt(pcSpec.GetRplIdentity(), false);

            pcSpec.SetControlledEntity(entity);
        }

        ChimeraCharacter character = ChimeraCharacter.Cast(entity);
        if (character)
        {
            entity.ClearFlags(EntityFlags.VISIBLE);
            
            Faction faction = null;
            if (m_sPlayerFactionKey != "")
            {
                FactionManager facMgr = GetGame().GetFactionManager();
                if (facMgr) 
                    faction = facMgr.GetFactionByKey(m_sPlayerFactionKey);
            }
            if (faction)
            {
                SCR_FactionAffiliationComponent.SetFaction(entity, faction);
            }
            else
            {
                Print("[LobbyMgr] ERROR: m_sPlayerFactionKey is empty or invalid! Voice will NOT work!", LogLevel.ERROR);
            }

            SCR_DamageManagerComponent dmgMgr = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
            if (dmgMgr) dmgMgr.FullHeal();
            
            SCR_CharacterControllerComponent charCtrl = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
            if (charCtrl)
            {
                charCtrl.SetDisableMovementControls(true); 
                charCtrl.SetDisableWeaponControls(true);   
            }
        }
    }

    protected void DeleteSpectatorForPlayer(int playerId)
    {
        IEntity spectator = m_mSpectators.Get(playerId);
        if (spectator)
        {
            SCR_EntityHelper.DeleteEntityAndChildren(spectator);
            m_mSpectators.Remove(playerId);
        }
    }

    protected void TransitionJIPToCamera(int playerId)
    {
        if (!Replication.IsServer()) return;
        IEntity dummy = m_mSpectators.Get(playerId);
        if (!dummy) return;
        
        vector spawnPos = dummy.GetOrigin();
        spawnPos[1] = spawnPos[1] + 50.0;

        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc) rpc.BroadcastJIPCameraSpawn(playerId, spawnPos);
    }

    protected IEntity SpawnDeathDummy(int playerId, vector pos)
    {
        if (m_sSpectatorPrefab == "") 
        {
            Print("[LobbyMgr] SpectatorPrefab is empty for death dummy!", LogLevel.ERROR);
            return null;
        }

        Resource res = Resource.Load(m_sSpectatorPrefab);
        if (!res || !res.IsValid()) return null;

        vector spawnMat[4];
        Math3D.MatrixIdentity4(spawnMat);
        spawnMat[3] = pos;

        EntitySpawnParams params = new EntitySpawnParams();
        params.TransformMode = ETransformMode.WORLD;
        params.Transform = spawnMat;

        IEntity entity = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
        if (!entity) return null;

        m_mSpectators.Set(playerId, entity);

        ChimeraCharacter character = ChimeraCharacter.Cast(entity);
        if (character)
        {
            entity.ClearFlags(EntityFlags.VISIBLE);
            
            SCR_CharacterControllerComponent charCtrl = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
            if (charCtrl)
            {
                charCtrl.SetDisableMovementControls(true); 
                charCtrl.SetDisableWeaponControls(true);   
            }
        }
        return entity;
    }

    override void EOnFrame(IEntity owner, float timeSlice)
    {
        if (!Replication.IsServer()) return;
        if (m_bLobbyActive) return;
        if (m_mPlayers.IsEmpty()) return;
        
        foreach (int pid, LobbyPlayerData data : m_mPlayers)
        {
            if (!data || m_aDeadPlayers.Contains(pid)) continue;
            
            bool isDead = false;
            if (data.m_CharacterEntity)
            {
                SCR_DamageManagerComponent dmgMgr = SCR_DamageManagerComponent.Cast(data.m_CharacterEntity.FindComponent(SCR_DamageManagerComponent));
                if (dmgMgr)
                {
                    SCR_HitZone hitZone = SCR_HitZone.Cast(dmgMgr.GetDefaultHitZone());
                    if (hitZone && hitZone.GetHealth() <= 0.0)
                        isDead = true;
                }
            }
            
            if (!data.m_CharacterEntity) 
                isDead = true;

            if (isDead)
            {
                m_aDeadPlayers.Insert(pid);
                
                vector deathPos = vector.Zero;
                if (data.m_CharacterEntity) deathPos = data.m_CharacterEntity.GetOrigin();
                
                PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(pid);
                if (pc)
                {
                    IEntity deathDummy = SpawnDeathDummy(pid, deathPos);
                    if (deathDummy)
                        pc.SetControlledEntity(deathDummy);
                }
                
                LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
                if (rpc) rpc.BroadcastJIPCameraSpawn(pid, deathPos);
            }
        }
    }

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        if (Replication.IsServer()) Print("[LobbyMgr] Server init.");
    }

    override void OnPlayerConnected(int playerId)
    {
        super.OnPlayerConnected(playerId);
        if (!Replication.IsServer()) return;

        if (!m_bLobbyActive) 
        {
            LobbyPlayerData data = m_mPlayers.Get(playerId);
            
            if (!data || !data.m_CharacterEntity)
            {
                Print("[LobbyMgr] Player " + playerId + " joined late (no character). Starting Camera Hijack phase...", LogLevel.NORMAL);
                LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
                if (rpc) rpc.BroadcastJIPSpectator(playerId);
                
                GetGame().GetCallqueue().CallLater(SpawnSpectatorForPlayer, 1000, false, playerId);
                GetGame().GetCallqueue().CallLater(TransitionJIPToCamera, 2500, false, playerId);
                return;
            }
            
            return;
        }

        m_mPlayers.Set(playerId, new LobbyPlayerData(playerId));
        GetGame().GetCallqueue().CallLater(SpawnSpectatorForPlayer, 1000, false, playerId);
        GetGame().GetCallqueue().CallLater(FetchPlayerName, 800, false, playerId);
    }

    override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
    {
        super.OnPlayerDisconnected(playerId, cause, timeout);
        if (!Replication.IsServer()) return;
        
        m_aDeadPlayers.Remove(playerId); 
        
        if (m_bLobbyActive)
        {
            DeleteSpectatorForPlayer(playerId);
            m_mPlayers.Remove(playerId);
        }
        
        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc) 
        { 
            rpc.BroadcastPlayerLeft(playerId); 
            rpc.BroadcastHeaderUpdate(); 
        }
    }

    static LobbyManagerComponent GetInstance()
    {
        SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (!gameMode) return null;
        return LobbyManagerComponent.Cast(gameMode.FindComponent(LobbyManagerComponent));
    }
}