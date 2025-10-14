// Fishing.cpp
#include "Fishing.h"
#include "Modules/ModuleManager.h"

// ★ 모든 로그 카테고리 정의 ★
DEFINE_LOG_CATEGORY(LogFishing);
DEFINE_LOG_CATEGORY(LogInventory);
DEFINE_LOG_CATEGORY(LogInventoryGrid);
DEFINE_LOG_CATEGORY(LogInventoryStorage);
DEFINE_LOG_CATEGORY(LogInventoryValidator);
DEFINE_LOG_CATEGORY(LogInventoryHandler);
DEFINE_LOG_CATEGORY(LogInventoryUI);
DEFINE_LOG_CATEGORY(LogInventoryNetwork);
DEFINE_LOG_CATEGORY(LogFishSpawnPool);
DEFINE_LOG_CATEGORY(LogFishingPC);

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, Fishing, "Fishing");