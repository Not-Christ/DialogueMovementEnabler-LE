#include "Settings.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

namespace
{
	constexpr const char* INI_PATH = R"(.\Data\SKSE\Plugins\DialogueMovementEnabler.ini)";

	FILETIME s_lastIniWriteTime{};
	bool s_hasLastWriteTime = false;

	std::string Trim(std::string a_str)
	{
		const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
		a_str.erase(a_str.begin(), std::find_if(a_str.begin(), a_str.end(), notSpace));
		a_str.erase(std::find_if(a_str.rbegin(), a_str.rend(), notSpace).base(), a_str.end());
		return a_str;
	}

	std::string ToLower(std::string a_str)
	{
		std::transform(a_str.begin(), a_str.end(), a_str.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		return a_str;
	}

	using IniData = std::map<std::string, std::map<std::string, std::string>>;

	bool ReadIniFile(const char* a_path, IniData& a_out)
	{
		std::ifstream file(a_path);
		if (!file.is_open()) {
			return false;
		}

		std::string section;
		std::string line;
		while (std::getline(file, line)) {
			// Strip UTF-8 BOM if present on first line
			if (line.size() >= 3 &&
				static_cast<unsigned char>(line[0]) == 0xEF &&
				static_cast<unsigned char>(line[1]) == 0xBB &&
				static_cast<unsigned char>(line[2]) == 0xBF) {
				line.erase(0, 3);
			}

			std::string trimmed = Trim(line);
			if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
				continue;
			}

			if (trimmed.front() == '[' && trimmed.back() == ']') {
				section = ToLower(Trim(trimmed.substr(1, trimmed.size() - 2)));
				continue;
			}

			const auto eqPos = trimmed.find('=');
			if (eqPos == std::string::npos) {
				continue;
			}

			std::string key = ToLower(Trim(trimmed.substr(0, eqPos)));
			std::string value = Trim(trimmed.substr(eqPos + 1));

			// Strip inline comments
			const auto commentPos = value.find_first_of("#;");
			if (commentPos != std::string::npos) {
				value = Trim(value.substr(0, commentPos));
			}

			if (!section.empty() && !key.empty()) {
				a_out[section][key] = value;
			}
		}

		return true;
	}

	bool GetIniBool(const IniData& a_ini, const char* a_section, const char* a_key, bool a_default)
	{
		const auto secIt = a_ini.find(ToLower(a_section));
		if (secIt == a_ini.end()) {
			return a_default;
		}
		const auto keyIt = secIt->second.find(ToLower(a_key));
		if (keyIt == secIt->second.end() || keyIt->second.empty()) {
			return a_default;
		}

		const std::string val = ToLower(keyIt->second);
		if (val == "true" || val == "1" || val == "yes" || val == "on") {
			return true;
		}
		if (val == "false" || val == "0" || val == "no" || val == "off") {
			return false;
		}
		return a_default;
	}

	float GetIniFloat(const IniData& a_ini, const char* a_section, const char* a_key, float a_default)
	{
		const auto secIt = a_ini.find(ToLower(a_section));
		if (secIt == a_ini.end()) {
			return a_default;
		}
		const auto keyIt = secIt->second.find(ToLower(a_key));
		if (keyIt == secIt->second.end() || keyIt->second.empty()) {
			return a_default;
		}

		char* endPtr = nullptr;
		const float parsed = std::strtof(keyIt->second.c_str(), &endPtr);
		if (endPtr == keyIt->second.c_str()) {
			return a_default;
		}
		return parsed;
	}

	std::uintptr_t GetIniAddress(const IniData& a_ini, const char* a_section, const char* a_key, std::uintptr_t a_default)
	{
		const auto secIt = a_ini.find(ToLower(a_section));
		if (secIt == a_ini.end()) {
			return a_default;
		}
		const auto keyIt = secIt->second.find(ToLower(a_key));
		if (keyIt == secIt->second.end() || keyIt->second.empty()) {
			return a_default;
		}

		char* endPtr = nullptr;
		const unsigned long parsed = std::strtoul(keyIt->second.c_str(), &endPtr, 0);
		if (endPtr == keyIt->second.c_str()) {
			return a_default;
		}
		return static_cast<std::uintptr_t>(parsed);
	}

	const char* BoolStr(bool a_value) noexcept
	{
		return a_value ? "true" : "false";
	}

	bool GetFileWriteTime(const char* a_path, FILETIME& a_out) noexcept
	{
		WIN32_FILE_ATTRIBUTE_DATA fad{};
		if (!GetFileAttributesExA(a_path, GetFileExInfoStandard, &fad)) {
			return false;
		}
		a_out = fad.ftLastWriteTime;
		return true;
	}

	void UpdateCachedWriteTime() noexcept
	{
		FILETIME ft{};
		if (GetFileWriteTime(INI_PATH, ft)) {
			s_lastIniWriteTime = ft;
			s_hasLastWriteTime = true;
		}
	}
}

namespace DME
{
	Settings* Settings::GetSingleton()
	{
		static Settings singleton;
		return &singleton;
	}

	void LoadSettings(bool a_saveBack)
	{
		using ControlType = Settings::ControlType;
		Settings* s = Settings::GetSingleton();

		IniData ini;
		const bool existed = ReadIniFile(INI_PATH, ini);

		SKSE::log::Write("Loading settings from %s (exists=%u)", INI_PATH, existed ? 1u : 0u);

		// [GENERAL]
		s->unlockCamera = GetIniBool(ini, "GENERAL", "bUnlockCamera", UNLOCK_CAMERA_DEFAULT_VALUE);
		s->freeLook = GetIniBool(ini, "GENERAL", "bFreeLook", FREE_LOOK_DEFAULT_VALUE);
		s->disableTurnToSpeakerPlayerInitiated = GetIniBool(ini, "GENERAL", "bDisableTurnToSpeakerPlayerInitiated", DISABLE_TURN_TO_SPEAKER_PLAYER_INITIATED_DEFAULT_VALUE);
		s->disableTurnToSpeakerNPCInitiated = GetIniBool(ini, "GENERAL", "bDisableTurnToSpeakerNPCInitiated", DISABLE_TURN_TO_SPEAKER_NPC_INITIATED_DEFAULT_VALUE);
		s->hotReloadINI = GetIniBool(ini, "GENERAL", "bHotReloadINI", HOT_RELOAD_INI_DEFAULT_VALUE);

		// [CONTROLS_KEYBOARD]
		s->allowMovement[ControlType::kKeyboardMouse] = GetIniBool(ini, "CONTROLS_KEYBOARD", "bAllowMovement", ALLOW_MOVEMENT_KEYBOARD_DEFAULT_VALUE);
		s->allowRun[ControlType::kKeyboardMouse] = GetIniBool(ini, "CONTROLS_KEYBOARD", "bAllowRun", ALLOW_RUN_KEYBOARD_DEFAULT_VALUE);
		s->allowToggleRun[ControlType::kKeyboardMouse] = GetIniBool(ini, "CONTROLS_KEYBOARD", "bAllowToggleRun", ALLOW_TOGGLE_RUN_KEYBOARD_DEFAULT_VALUE);
		s->allowJump[ControlType::kKeyboardMouse] = GetIniBool(ini, "CONTROLS_KEYBOARD", "bAllowJump", ALLOW_JUMP_KEYBOARD_DEFAULT_VALUE);
		s->allowSprint[ControlType::kKeyboardMouse] = GetIniBool(ini, "CONTROLS_KEYBOARD", "bAllowSprint", ALLOW_SPRINT_KEYBOARD_DEFAULT_VALUE);
		s->allowTogglePOV[ControlType::kKeyboardMouse] = GetIniBool(ini, "CONTROLS_KEYBOARD", "bAllowTogglePOV", ALLOW_TOGGLE_POV_KEYBOARD_DEFAULT_VALUE);
		s->allowSneak[ControlType::kKeyboardMouse] = GetIniBool(ini, "CONTROLS_KEYBOARD", "bAllowSneak", ALLOW_SNEAK_KEYBOARD_DEFAULT_VALUE);
		s->allowReadyWeapon[ControlType::kKeyboardMouse] = GetIniBool(ini, "CONTROLS_KEYBOARD", "bAllowReadyWeapon", ALLOW_READY_WEAPON_KEYBOARD_DEFAULT_VALUE);
		s->allowLeftAttack[ControlType::kKeyboardMouse] = GetIniBool(ini, "CONTROLS_KEYBOARD", "bAllowLeftAttack", ALLOW_LEFT_ATTACK_KEYBOARD_DEFAULT_VALUE);
		s->allowRightAttack[ControlType::kKeyboardMouse] = GetIniBool(ini, "CONTROLS_KEYBOARD", "bAllowRightAttack", ALLOW_RIGHT_ATTACK_KEYBOARD_DEFAULT_VALUE);
		s->allowHotkeys[ControlType::kKeyboardMouse] = GetIniBool(ini, "CONTROLS_KEYBOARD", "bAllowHotkeys", ALLOW_HOTKEYS_KEYBOARD_DEFAULT_VALUE);
		s->allowShout[ControlType::kKeyboardMouse] = GetIniBool(ini, "CONTROLS_KEYBOARD", "bAllowShout", ALLOW_SHOUT_KEYBOARD_DEFAULT_VALUE);

		// [CONTROLS_CONTROLLER]
		s->allowMovement[ControlType::kController] = GetIniBool(ini, "CONTROLS_CONTROLLER", "bAllowMovement", ALLOW_MOVEMENT_CONTROLLER_DEFAULT_VALUE);
		s->allowRun[ControlType::kController] = GetIniBool(ini, "CONTROLS_CONTROLLER", "bAllowRun", ALLOW_RUN_CONTROLLER_DEFAULT_VALUE);
		s->allowToggleRun[ControlType::kController] = GetIniBool(ini, "CONTROLS_CONTROLLER", "bAllowToggleRun", ALLOW_TOGGLE_RUN_CONTROLLER_DEFAULT_VALUE);
		s->allowJump[ControlType::kController] = GetIniBool(ini, "CONTROLS_CONTROLLER", "bAllowJump", ALLOW_JUMP_CONTROLLER_DEFAULT_VALUE);
		s->allowSprint[ControlType::kController] = GetIniBool(ini, "CONTROLS_CONTROLLER", "bAllowSprint", ALLOW_SPRINT_CONTROLLER_DEFAULT_VALUE);
		s->allowTogglePOV[ControlType::kController] = GetIniBool(ini, "CONTROLS_CONTROLLER", "bAllowTogglePOV", ALLOW_TOGGLE_POV_CONTROLLER_DEFAULT_VALUE);
		s->allowSneak[ControlType::kController] = GetIniBool(ini, "CONTROLS_CONTROLLER", "bAllowSneak", ALLOW_SNEAK_CONTROLLER_DEFAULT_VALUE);
		s->allowReadyWeapon[ControlType::kController] = GetIniBool(ini, "CONTROLS_CONTROLLER", "bAllowReadyWeapon", ALLOW_READY_WEAPON_CONTROLLER_DEFAULT_VALUE);
		s->allowLeftAttack[ControlType::kController] = GetIniBool(ini, "CONTROLS_CONTROLLER", "bAllowLeftAttack", ALLOW_LEFT_ATTACK_CONTROLLER_DEFAULT_VALUE);
		s->allowRightAttack[ControlType::kController] = GetIniBool(ini, "CONTROLS_CONTROLLER", "bAllowRightAttack", ALLOW_RIGHT_ATTACK_CONTROLLER_DEFAULT_VALUE);
		s->allowHotkeys[ControlType::kController] = GetIniBool(ini, "CONTROLS_CONTROLLER", "bAllowHotkeys", ALLOW_HOTKEYS_CONTROLLER_DEFAULT_VALUE);
		s->allowShout[ControlType::kController] = GetIniBool(ini, "CONTROLS_CONTROLLER", "bAllowShout", ALLOW_SHOUT_CONTROLLER_DEFAULT_VALUE);

		// [AUTOCLOSE]
		s->autoCloseMenus = GetIniBool(ini, "AUTOCLOSE", "bAutoCloseMenus", AUTO_CLOSE_MENUS_DEFAULT_VALUE);
		s->autoCloseDistance = GetIniFloat(ini, "AUTOCLOSE", "fAutoCloseDistance", AUTO_CLOSE_DISTANCE_DEFAULT_VALUE);
		s->autoCloseTolerance = GetIniFloat(ini, "AUTOCLOSE", "fAutoCloseTolerance", AUTO_CLOSE_TOLERANCE_DEFAULT_VALUE);

		// [HOOKS] (optional LE call-site overrides; 0 = auto-discover)
		s->hookGetDialogueLookAngle = GetIniAddress(ini, "HOOKS", "uGetDialogueLookAngleCallSite", 0);
		s->hookStartDialogueForceGreet = GetIniAddress(ini, "HOOKS", "uStartDialogueForceGreetCallSite", 0);
		s->hookInitDialogueLookAt = GetIniAddress(ini, "HOOKS", "uInitDialogueLookAtCallSite", 0);
		s->hookIsGamepadEnabled = GetIniAddress(ini, "HOOKS", "uIsGamepadEnabledCallSite", 0);

		SKSE::log::Write(
			"Settings: unlockCamera=%u freeLook=%u disableTurnPlayer=%u disableTurnNPC=%u hotReload=%u autoClose=%u dist=%.1f tol=%.1f",
			s->unlockCamera ? 1u : 0u,
			s->freeLook ? 1u : 0u,
			s->disableTurnToSpeakerPlayerInitiated ? 1u : 0u,
			s->disableTurnToSpeakerNPCInitiated ? 1u : 0u,
			s->hotReloadINI ? 1u : 0u,
			s->autoCloseMenus ? 1u : 0u,
			s->autoCloseDistance,
			s->autoCloseTolerance);

		if (a_saveBack && !existed) {
			SaveSettings();
		} else {
			UpdateCachedWriteTime();
		}
	}

	void SaveSettings()
	{
		using ControlType = Settings::ControlType;
		const Settings* s = Settings::GetSingleton();

		std::error_code ec;
		const std::filesystem::path iniPath(INI_PATH);
		if (iniPath.has_parent_path()) {
			std::filesystem::create_directories(iniPath.parent_path(), ec);
		}

		std::ofstream out(INI_PATH, std::ios::trunc);
		if (!out.is_open()) {
			SKSE::log::Write("ERROR: Failed to write settings to %s", INI_PATH);
			return;
		}

		out << std::fixed << std::setprecision(6);
		out << "[GENERAL]\n\n"
			<< "#  Unlocks camera rotation so you can look around in any direction.\n"
			<< "bUnlockCamera = " << BoolStr(s->unlockCamera) << "\n\n"
			<< "#  Turns the camera with the mouse instead of by moving the cursor to the edge of the screen.\n"
			<< "#  You can use the scroll wheel to select options. Note that this may not work on every configuration.\n"
			<< "#  This setting implies bUnlockCamera is enabled.\n"
			<< "bFreeLook = " << BoolStr(s->freeLook) << "\n\n"
			<< "#  Stops the camera from turning to face the speaker when you start a conversation.\n"
			<< "bDisableTurnToSpeakerPlayerInitiated = " << BoolStr(s->disableTurnToSpeakerPlayerInitiated) << "\n\n"
			<< "#  Stops the camera from turning to face the speaker when an NPC starts a conversation with you.\n"
			<< "#  Not recommended, as you may not be able to tell who is talking to you.\n"
			<< "bDisableTurnToSpeakerNPCInitiated = " << BoolStr(s->disableTurnToSpeakerNPCInitiated) << "\n\n"
			<< "#  Automatically reload this INI when opening dialogue if the file was modified (enables live editing and SkyUI MCM sync).\n"
			<< "bHotReloadINI = " << BoolStr(s->hotReloadINI) << "\n\n\n";

		out << "[CONTROLS_KEYBOARD]\n"
			<< "bAllowMovement = " << BoolStr(s->allowMovement[ControlType::kKeyboardMouse]) << "\n"
			<< "bAllowRun = " << BoolStr(s->allowRun[ControlType::kKeyboardMouse]) << "\n"
			<< "bAllowToggleRun = " << BoolStr(s->allowToggleRun[ControlType::kKeyboardMouse]) << "\n"
			<< "bAllowJump = " << BoolStr(s->allowJump[ControlType::kKeyboardMouse]) << "\n"
			<< "bAllowSprint = " << BoolStr(s->allowSprint[ControlType::kKeyboardMouse]) << "\n"
			<< "bAllowTogglePOV = " << BoolStr(s->allowTogglePOV[ControlType::kKeyboardMouse]) << "\n"
			<< "bAllowSneak = " << BoolStr(s->allowSneak[ControlType::kKeyboardMouse]) << "\n"
			<< "bAllowReadyWeapon = " << BoolStr(s->allowReadyWeapon[ControlType::kKeyboardMouse]) << "\n"
			<< "bAllowLeftAttack = " << BoolStr(s->allowLeftAttack[ControlType::kKeyboardMouse]) << "\n"
			<< "bAllowRightAttack = " << BoolStr(s->allowRightAttack[ControlType::kKeyboardMouse]) << "\n"
			<< "bAllowHotkeys = " << BoolStr(s->allowHotkeys[ControlType::kKeyboardMouse]) << "\n"
			<< "bAllowShout = " << BoolStr(s->allowShout[ControlType::kKeyboardMouse]) << "\n\n\n";

		out << "[CONTROLS_CONTROLLER]\n"
			<< "bAllowMovement = " << BoolStr(s->allowMovement[ControlType::kController]) << "\n"
			<< "bAllowRun = " << BoolStr(s->allowRun[ControlType::kController]) << "\n"
			<< "bAllowToggleRun = " << BoolStr(s->allowToggleRun[ControlType::kController]) << "\n"
			<< "bAllowJump = " << BoolStr(s->allowJump[ControlType::kController]) << "\n"
			<< "bAllowSprint = " << BoolStr(s->allowSprint[ControlType::kController]) << "\n"
			<< "bAllowTogglePOV = " << BoolStr(s->allowTogglePOV[ControlType::kController]) << "\n"
			<< "bAllowSneak = " << BoolStr(s->allowSneak[ControlType::kController]) << "\n"
			<< "bAllowReadyWeapon = " << BoolStr(s->allowReadyWeapon[ControlType::kController]) << "\n"
			<< "bAllowLeftAttack = " << BoolStr(s->allowLeftAttack[ControlType::kController]) << "\n"
			<< "bAllowRightAttack = " << BoolStr(s->allowRightAttack[ControlType::kController]) << "\n"
			<< "bAllowHotkeys = " << BoolStr(s->allowHotkeys[ControlType::kController]) << "\n"
			<< "bAllowShout = " << BoolStr(s->allowShout[ControlType::kController]) << "\n\n\n";

		out << "[AUTOCLOSE]\n\n"
			<< "#  Enable or disable auto-closing of the dialogue menu if the distance to the speaker is too large.\n"
			<< "bAutoCloseMenus = " << BoolStr(s->autoCloseMenus) << "\n\n"
			<< "#  The maximum distance (in in-game units) before the menu will automatically close.\n"
			<< "#  Too small values can cause the menus to close unexpectedly.\n"
			<< "fAutoCloseDistance = " << s->autoCloseDistance << "\n\n"
			<< "#  The maximum distance (in in-game units) where the menu NEVER auto-closes (in relation to the players initial position when the menu was opened).\n"
			<< "#  This is used as a failsafe when the initial distance is larger than the maximum allowed to prevent the menu from closing unexpectedly.\n"
			<< "fAutoCloseTolerance = " << s->autoCloseTolerance << "\n\n\n";

		out << "[HOOKS]\n\n"
			<< "#  Optional TESV.exe 1.9.32.0 call-site overrides (hex, e.g. 0x00773380). Leave at 0x0 for automatic runtime discovery.\n"
			<< "uGetDialogueLookAngleCallSite = 0x" << std::hex << std::uppercase << s->hookGetDialogueLookAngle << "\n"
			<< "uStartDialogueForceGreetCallSite = 0x" << s->hookStartDialogueForceGreet << "\n"
			<< "uInitDialogueLookAtCallSite = 0x" << s->hookInitDialogueLookAt << "\n"
			<< "uIsGamepadEnabledCallSite = 0x" << s->hookIsGamepadEnabled << std::dec << "\n";

		out.close();
		UpdateCachedWriteTime();
		SKSE::log::Write("Settings saved to %s", INI_PATH);
	}

	void RestoreDefaults()
	{
		using ControlType = Settings::ControlType;
		Settings* s = Settings::GetSingleton();

		s->unlockCamera = UNLOCK_CAMERA_DEFAULT_VALUE;
		s->freeLook = FREE_LOOK_DEFAULT_VALUE;
		s->disableTurnToSpeakerPlayerInitiated = DISABLE_TURN_TO_SPEAKER_PLAYER_INITIATED_DEFAULT_VALUE;
		s->disableTurnToSpeakerNPCInitiated = DISABLE_TURN_TO_SPEAKER_NPC_INITIATED_DEFAULT_VALUE;
		s->hotReloadINI = HOT_RELOAD_INI_DEFAULT_VALUE;

		s->allowMovement[ControlType::kKeyboardMouse] = ALLOW_MOVEMENT_KEYBOARD_DEFAULT_VALUE;
		s->allowRun[ControlType::kKeyboardMouse] = ALLOW_RUN_KEYBOARD_DEFAULT_VALUE;
		s->allowToggleRun[ControlType::kKeyboardMouse] = ALLOW_TOGGLE_RUN_KEYBOARD_DEFAULT_VALUE;
		s->allowJump[ControlType::kKeyboardMouse] = ALLOW_JUMP_KEYBOARD_DEFAULT_VALUE;
		s->allowSprint[ControlType::kKeyboardMouse] = ALLOW_SPRINT_KEYBOARD_DEFAULT_VALUE;
		s->allowTogglePOV[ControlType::kKeyboardMouse] = ALLOW_TOGGLE_POV_KEYBOARD_DEFAULT_VALUE;
		s->allowSneak[ControlType::kKeyboardMouse] = ALLOW_SNEAK_KEYBOARD_DEFAULT_VALUE;
		s->allowReadyWeapon[ControlType::kKeyboardMouse] = ALLOW_READY_WEAPON_KEYBOARD_DEFAULT_VALUE;
		s->allowLeftAttack[ControlType::kKeyboardMouse] = ALLOW_LEFT_ATTACK_KEYBOARD_DEFAULT_VALUE;
		s->allowRightAttack[ControlType::kKeyboardMouse] = ALLOW_RIGHT_ATTACK_KEYBOARD_DEFAULT_VALUE;
		s->allowHotkeys[ControlType::kKeyboardMouse] = ALLOW_HOTKEYS_KEYBOARD_DEFAULT_VALUE;
		s->allowShout[ControlType::kKeyboardMouse] = ALLOW_SHOUT_KEYBOARD_DEFAULT_VALUE;

		s->allowMovement[ControlType::kController] = ALLOW_MOVEMENT_CONTROLLER_DEFAULT_VALUE;
		s->allowRun[ControlType::kController] = ALLOW_RUN_CONTROLLER_DEFAULT_VALUE;
		s->allowToggleRun[ControlType::kController] = ALLOW_TOGGLE_RUN_CONTROLLER_DEFAULT_VALUE;
		s->allowJump[ControlType::kController] = ALLOW_JUMP_CONTROLLER_DEFAULT_VALUE;
		s->allowSprint[ControlType::kController] = ALLOW_SPRINT_CONTROLLER_DEFAULT_VALUE;
		s->allowTogglePOV[ControlType::kController] = ALLOW_TOGGLE_POV_CONTROLLER_DEFAULT_VALUE;
		s->allowSneak[ControlType::kController] = ALLOW_SNEAK_CONTROLLER_DEFAULT_VALUE;
		s->allowReadyWeapon[ControlType::kController] = ALLOW_READY_WEAPON_CONTROLLER_DEFAULT_VALUE;
		s->allowLeftAttack[ControlType::kController] = ALLOW_LEFT_ATTACK_CONTROLLER_DEFAULT_VALUE;
		s->allowRightAttack[ControlType::kController] = ALLOW_RIGHT_ATTACK_CONTROLLER_DEFAULT_VALUE;
		s->allowHotkeys[ControlType::kController] = ALLOW_HOTKEYS_CONTROLLER_DEFAULT_VALUE;
		s->allowShout[ControlType::kController] = ALLOW_SHOUT_CONTROLLER_DEFAULT_VALUE;

		s->autoCloseMenus = AUTO_CLOSE_MENUS_DEFAULT_VALUE;
		s->autoCloseDistance = AUTO_CLOSE_DISTANCE_DEFAULT_VALUE;
		s->autoCloseTolerance = AUTO_CLOSE_TOLERANCE_DEFAULT_VALUE;

		SaveSettings();
	}

	bool ReloadSettingsIfModified()
	{
		if (!Settings::GetSingleton()->hotReloadINI) {
			return false;
		}

		FILETIME currentFt{};
		if (!GetFileWriteTime(INI_PATH, currentFt)) {
			return false;
		}

		if (s_hasLastWriteTime &&
			currentFt.dwLowDateTime == s_lastIniWriteTime.dwLowDateTime &&
			currentFt.dwHighDateTime == s_lastIniWriteTime.dwHighDateTime) {
			return false;
		}

		SKSE::log::Write("Detected INI modification on disk; hot-reloading settings.");
		LoadSettings(false);
		return true;
	}
}
