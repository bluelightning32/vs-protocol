#include "client_id.h"

namespace vintage_story {

const std::unordered_map<int, const char *> client_ids {
  {33, "LoginTokenQuery"},
  {1, "PlayerIdentification"},
  {2, "PingReply"},
  {3, "BlockPlaceOrBreak"},
  {4, "ChatLine"},
  {7, "ActivateInventorySlot"},
  {8, "MoveItemstack"},
  {9, "FlipItemstacks"},
  {10, "CreateItemstack"},
  {11, "RequestJoin"},
  {12, "SpecialKey"},
  {13, "SelectedHotbarSlot"},
  {14, "Leave"},
  {15, "ServerQuery"},

  {17, "EntityInteraction"},
  {19, "PlayerPosition"},
  {20, "RequestModeChange"},
  {21, "MoveKeyChange"},

  {22, "BlockEntityPacket"},
  {31, "EntityPacket"},
  {23, "CustomPacket"},
  {25, "HandInteraction"},
  {26, "ClientLoaded"},
  {27, "SetToolMode"},
  {28, "BlockDamage"},
  {29, "ClientPlaying"},
  {30, "InvOpenClose"},
  {32, "RuntimeSetting"},
};

} //  namespace vintage_story
