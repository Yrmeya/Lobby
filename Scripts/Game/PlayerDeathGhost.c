[ComponentEditorProps(category: "GameScripted/Player", description: "Handles player death and spawns ghost camera.")]
class PlayerDeathGhostClass : ScriptComponentClass {}

class PlayerDeathGhost : ScriptComponent
{
    static ref map<int, IEntity> s_PlayerGhostCameras = new map<int, IEntity>();
    static ref set<int> s_DeadPlayers = new set<int>();

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        // Локальный перехват смерти убран. 
        // Теперь смерть обрабатывается на сервере в LobbyManagerComponent::EOnFrame,
        // чтобы избежать деспинка и гонки между клиентом и сервером.
    }

    static void SpawnGhostStatic(int playerId, vector origin)
    {
        Print("[GhostLog] === SpawnGhostStatic STARTED ===", LogLevel.WARNING);
        Print(string.Format("[GhostLog] Target player ID: %1", playerId), LogLevel.NORMAL);

        IEntity existingCamera = s_PlayerGhostCameras.Get(playerId);
        if (existingCamera)
        {
            Print("[GhostLog] Deleting old camera", LogLevel.NORMAL);
            delete existingCamera;
            s_PlayerGhostCameras.Remove(playerId);
        }

        PlayerDeathGhost.KillEditorUI();

        PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
        if (!pc) pc = GetGame().GetPlayerController();

        if (!pc)
        {
            Print("[GhostLog] ERROR: PlayerController lost! Retrying...", LogLevel.ERROR);
            GetGame().GetCallqueue().CallLater(PlayerDeathGhost.SpawnGhostStatic, 100, false, playerId, origin);
            return;
        }

        Print("[GhostLog] PlayerController found successfully", LogLevel.NORMAL);

        ResourceName ghostPrefabName = "{E1FF38EC8894C5F3}Prefabs/Editor/Camera/ManualCameraSpectate.et";
        Print(string.Format("[GhostLog] Loading prefab: %1", ghostPrefabName), LogLevel.NORMAL);

        Resource ghostResource = Resource.Load(ghostPrefabName);

        if (!ghostResource || !ghostResource.IsValid())
        {
            Print("[GhostLog] ERROR: Camera prefab not found or invalid!", LogLevel.ERROR);
            return;
        }

        Print("[GhostLog] Prefab resource loaded successfully", LogLevel.NORMAL);

        EntitySpawnParams params = new EntitySpawnParams();
        params.Transform[3] = origin + "0 2 0";

        Print(string.Format("[GhostLog] Spawn position: %1", params.Transform[3]), LogLevel.NORMAL);

        IEntity ghostEntity = GetGame().SpawnEntityPrefab(ghostResource, GetGame().GetWorld(), params);

        if (!ghostEntity)
        {
            Print("[GhostLog] ERROR: SpawnEntityPrefab failed!", LogLevel.ERROR);
            return;
        }

        Print(string.Format("[GhostLog] Entity spawned! Type: %1", ghostEntity.Type()), LogLevel.NORMAL);

        s_PlayerGhostCameras.Set(playerId, ghostEntity);
        
        if (!s_DeadPlayers.Contains(playerId))
        {
            s_DeadPlayers.Insert(playerId);
            Print(string.Format("[GhostLog] Player %1 marked as dead", playerId), LogLevel.WARNING);
        }

        // ИСПРАВЛЕНИЕ: Мы НЕ делаем pc.SetControlledEntity(ghostEntity);
        // Сервер уже посадил нас в невидимого манекена. Мы здесь только переключаем ВИД (CameraManager).
        Print("[GhostLog] Skipping SetControlledEntity (Server handles authority)", LogLevel.NORMAL);

        SCR_CameraManager cameraManager = SCR_CameraManager.Cast(GetGame().GetCameraManager());
        if (cameraManager)
        {
            CameraBase cam = CameraBase.Cast(ghostEntity);
            if (cam)
            {
                cameraManager.SetCamera(cam);
                Print("[GhostLog] Camera view set successfully via CameraManager", LogLevel.NORMAL);
            }
            else
            {
                Print("[GhostLog] WARNING: ghostEntity is not a CameraBase!", LogLevel.WARNING);
            }
        }
        else
        {
            Print("[GhostLog] ERROR: CameraManager not found!", LogLevel.ERROR);
        }

        Print("[GhostLog] === SUCCESS: Ghost Spectator Active ===", LogLevel.WARNING);

        GetGame().GetCallqueue().CallLater(PlayerDeathGhost.KillEditorUI, 100, false);
        GetGame().GetCallqueue().CallLater(PlayerDeathGhost.KillEditorUI, 500, false);
    }

    static bool IsPlayerDead(int playerId)
    {
        return s_DeadPlayers.Contains(playerId);
    }

    static void RevivePlayer(int playerId)
    {
        Print(string.Format("[GhostLog] Reviving player %1", playerId), LogLevel.NORMAL);
        
        s_DeadPlayers.Remove(playerId);
        
        IEntity camera = s_PlayerGhostCameras.Get(playerId);
        if (camera)
        {
            delete camera;
            s_PlayerGhostCameras.Remove(playerId);
        }
    }

    static void KillEditorUI()
    {
        SCR_EditorManagerCore editorCore = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
        if (editorCore)
        {
            SCR_EditorManagerEntity editorManager = editorCore.GetEditorManager();
            if (editorManager)
            {
                if (editorManager.IsOpened()) editorManager.Close();
            }
        }

        MenuManager menuManager = GetGame().GetMenuManager();
        if (menuManager)
        {
            menuManager.CloseAllMenus();
        }

        GetGame().GetInputManager().ActivateContext("ManualCameraContext");
    }
}