#include "server_id.h"

namespace vintage_story {

const std::unordered_map<int, const char *> server_ids {
  {77, "TokenAnswer"},
  {1, "ServerIdentification"},
  {2, "Ping"},
  {3, "PlayerPing"},
  {4, "LevelInitialize"},
  {5, "LevelDataChunk"},
  {6, "LevelFinalize"},
  {7, "SetBlock"},
  {8, "ChatLine"},
  {9, "DisconnectPlayer"},
  {10, "Chunks"},
  {11, "UnloadServerChunk"},
  {12, "Inventory"},
  {13, "Calendar"},
  {17, "MapChunk"},
  {18, "Sound"},
  {19, "ServerAssets"},
  {21, "WorldMetaData"},
  {24, "BlockType"},
  {28, "QueryAnswer"},
  {29, "ServerRedirect"},
  {30, "InventoryContents"},
  {31, "InventoryUpdate"},
  {32, "InventoryDoubleUpdate"},
  {33, "Entity"},
  {34, "EntitySpawn"},
  {35, "EntityMoved"},
  {36, "EntityDespawn"},
  {37, "EntityAttributes"},
  {38, "EntityAttributeUpdate"},
  {67, "EntityPacket"},

  {40, "Entities"},
  {41, "PlayerData"},
  {42, "MapRegion"},

  {44, "BlockEntityMessage"},
  {45, "PlayerDeath"},
  {46, "ModeChange"},
  {47, "SetBlocks"},
  {48, "BlockEntities"},
  {49, "PlayerGroups"},
  {50, "PlayerGroup"},
  {51, "SpawnPosition"},
  {52, "HighlightBlocks"},
  {53, "SelectedHotbarSlot"},
  {55, "CustomPacket"},
  {56, "NetworkChannels"},
  {57, "GotoGroup"},
  {58, "ExchangeBlock"},
  {59, "StopMovement"},
  {60, "EntityBulkAttributes"},
  {61, "SpawnParticles"},
  {62, "EntityBulkDebugAttributes"},
  {63, "SetBlocksNoRelight"},
  {64, "BlockDamage"},
  {65, "Ambient"},
  {66, "NotifySlot"},
  {68, "IngameError"},
  {69, "IngameDiscovery"},
  {70, "SetBlocksMinimal"},
  {71, "SetDecors"},
  {72, "RemoveBlockLight"},
  {73, "ServerReady"},
  {74, "UnloadMapRegion"},
  {75, "LandClaims"},
  {76, "Roles"},
};

} //  namespace vintage_story