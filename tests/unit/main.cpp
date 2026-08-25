/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2023 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "config/configmanager.hpp"
#include "database/database.hpp"
#include "lib/di/container.hpp"
#include "lib/logging/in_memory_logger.hpp"

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);

	// The injected singletons reference each other during destruction (for
	// example Game owns Lua script interfaces which request LuaEnvironment).
	// Keep the test container alive until process termination so its unordered
	// singleton storage is never partially torn down underneath a destructor.
	auto* injector = new di::extension::injector<>();
	InMemoryLogger::install(*injector);
	DI::setTestContainer(injector);

	(void)g_logger();
	(void)g_configManager();
	(void)g_database();

	return RUN_ALL_TESTS();
}
