#include "Hooks.h"
#include "Settings.h"
#include "Version.h"

namespace
{
	void MessageHandler(SKSE::MessagingInterface::Message* a_message)
	{
		if (!a_message) {
			return;
		}

		switch (a_message->type) {
		case SKSE::MessagingInterface::kPostLoadGame:
		case SKSE::MessagingInterface::kNewGame:
		case SKSE::MessagingInterface::kDataLoaded:
			DME::ReloadSettingsIfModified();
			break;
		default:
			break;
		}
	}
}

extern "C"
{
	DLLEXPORT bool SKSEPlugin_Query(const SKSE::LoadInterface* a_skse, SKSE::PluginInfo* a_info)
	{
		if (!a_skse || !a_info) {
			return false;
		}

		a_info->infoVersion = SKSE::PluginInfo::kInfoVersion;
		a_info->name = Version::NAME.data();
		a_info->version = static_cast<std::uint32_t>(
			(Version::MAJOR << 16) | (Version::MINOR << 8) | Version::PATCH);

		if (a_skse->isEditor) {
			return false;
		}

		if (a_skse->runtimeVersion != RE::RUNTIME_VERSION_1_9_32_0) {
			return false;
		}

		return true;
	}

	DLLEXPORT bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
	{
		if (!SKSE::Init(a_skse)) {
			return false;
		}

		SKSE::log::Open(Version::NAME.data());
		SKSE::log::Write("%s v%s (%s)", Version::FORMATTED_NAME.data(), Version::STRING.data(), __TIMESTAMP__);

		if (!REL::Trampoline::get().create(1024)) {
			SKSE::log::Write("CRITICAL: Failed to create REL::Trampoline.");
			return false;
		}

		if (auto* messaging = SKSE::GetMessagingInterface()) {
			if (messaging->interfaceVersion >= SKSE::MessagingInterface::kInterfaceVersion &&
				messaging->RegisterListener(SKSE::GetPluginHandle(), "SKSE", MessageHandler)) {
				SKSE::log::Write("SKSE messaging listener registered.");
			}
		}

		DME::LoadSettings(true);
		DME::InstallHooks();

		SKSE::log::Write("Dialogue Movement Enabler LE loaded successfully.");
		return true;
	}
}

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID)
{
	return TRUE;
}
