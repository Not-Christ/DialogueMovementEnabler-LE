#pragma once

#include <cstdint>

namespace DME
{
	// General
	constexpr bool UNLOCK_CAMERA_DEFAULT_VALUE = true;
	constexpr bool FREE_LOOK_DEFAULT_VALUE = false;
	constexpr bool DISABLE_TURN_TO_SPEAKER_PLAYER_INITIATED_DEFAULT_VALUE = false;
	constexpr bool DISABLE_TURN_TO_SPEAKER_NPC_INITIATED_DEFAULT_VALUE = false;
	constexpr bool HOT_RELOAD_INI_DEFAULT_VALUE = true;

	// Controls (Keyboard)
	constexpr bool ALLOW_MOVEMENT_KEYBOARD_DEFAULT_VALUE = true;
	constexpr bool ALLOW_RUN_KEYBOARD_DEFAULT_VALUE = true;
	constexpr bool ALLOW_TOGGLE_RUN_KEYBOARD_DEFAULT_VALUE = true;
	constexpr bool ALLOW_JUMP_KEYBOARD_DEFAULT_VALUE = false;
	constexpr bool ALLOW_SPRINT_KEYBOARD_DEFAULT_VALUE = false;
	constexpr bool ALLOW_TOGGLE_POV_KEYBOARD_DEFAULT_VALUE = false;
	constexpr bool ALLOW_SNEAK_KEYBOARD_DEFAULT_VALUE = false;
	constexpr bool ALLOW_READY_WEAPON_KEYBOARD_DEFAULT_VALUE = false;
	constexpr bool ALLOW_LEFT_ATTACK_KEYBOARD_DEFAULT_VALUE = false;
	constexpr bool ALLOW_RIGHT_ATTACK_KEYBOARD_DEFAULT_VALUE = false;
	constexpr bool ALLOW_HOTKEYS_KEYBOARD_DEFAULT_VALUE = false;
	constexpr bool ALLOW_SHOUT_KEYBOARD_DEFAULT_VALUE = false;

	// Controls (Controller)
	constexpr bool ALLOW_MOVEMENT_CONTROLLER_DEFAULT_VALUE = true;
	constexpr bool ALLOW_RUN_CONTROLLER_DEFAULT_VALUE = true;
	constexpr bool ALLOW_TOGGLE_RUN_CONTROLLER_DEFAULT_VALUE = true;
	constexpr bool ALLOW_JUMP_CONTROLLER_DEFAULT_VALUE = false;
	constexpr bool ALLOW_SPRINT_CONTROLLER_DEFAULT_VALUE = false;
	constexpr bool ALLOW_TOGGLE_POV_CONTROLLER_DEFAULT_VALUE = false;
	constexpr bool ALLOW_SNEAK_CONTROLLER_DEFAULT_VALUE = false;
	constexpr bool ALLOW_READY_WEAPON_CONTROLLER_DEFAULT_VALUE = false;
	constexpr bool ALLOW_LEFT_ATTACK_CONTROLLER_DEFAULT_VALUE = false;
	constexpr bool ALLOW_RIGHT_ATTACK_CONTROLLER_DEFAULT_VALUE = false;
	constexpr bool ALLOW_HOTKEYS_CONTROLLER_DEFAULT_VALUE = false;
	constexpr bool ALLOW_SHOUT_CONTROLLER_DEFAULT_VALUE = false;

	// Auto-close
	constexpr bool AUTO_CLOSE_MENUS_DEFAULT_VALUE = true;
	constexpr float AUTO_CLOSE_DISTANCE_DEFAULT_VALUE = 1200.0f;
	constexpr float AUTO_CLOSE_TOLERANCE_DEFAULT_VALUE = 300.0f;

	class Settings
	{
	public:
		enum ControlType
		{
			kKeyboardMouse = 0,
			kController = 1,

			kTotal
		};

		static Settings* GetSingleton();

		// General
		bool unlockCamera{ UNLOCK_CAMERA_DEFAULT_VALUE };
		bool freeLook{ FREE_LOOK_DEFAULT_VALUE };  // Implies unlockCamera
		bool disableTurnToSpeakerPlayerInitiated{ DISABLE_TURN_TO_SPEAKER_PLAYER_INITIATED_DEFAULT_VALUE };
		bool disableTurnToSpeakerNPCInitiated{ DISABLE_TURN_TO_SPEAKER_NPC_INITIATED_DEFAULT_VALUE };
		bool hotReloadINI{ HOT_RELOAD_INI_DEFAULT_VALUE };

		// Controls
		bool allowMovement[kTotal]{ ALLOW_MOVEMENT_KEYBOARD_DEFAULT_VALUE, ALLOW_MOVEMENT_CONTROLLER_DEFAULT_VALUE };
		bool allowJump[kTotal]{ ALLOW_JUMP_KEYBOARD_DEFAULT_VALUE, ALLOW_JUMP_CONTROLLER_DEFAULT_VALUE };
		bool allowSprint[kTotal]{ ALLOW_SPRINT_KEYBOARD_DEFAULT_VALUE, ALLOW_SPRINT_CONTROLLER_DEFAULT_VALUE };
		bool allowRun[kTotal]{ ALLOW_RUN_KEYBOARD_DEFAULT_VALUE, ALLOW_RUN_CONTROLLER_DEFAULT_VALUE };
		bool allowToggleRun[kTotal]{ ALLOW_TOGGLE_RUN_KEYBOARD_DEFAULT_VALUE, ALLOW_TOGGLE_RUN_CONTROLLER_DEFAULT_VALUE };
		bool allowSneak[kTotal]{ ALLOW_SNEAK_KEYBOARD_DEFAULT_VALUE, ALLOW_SNEAK_CONTROLLER_DEFAULT_VALUE };
		bool allowTogglePOV[kTotal]{ ALLOW_TOGGLE_POV_KEYBOARD_DEFAULT_VALUE, ALLOW_TOGGLE_POV_CONTROLLER_DEFAULT_VALUE };
		bool allowReadyWeapon[kTotal]{ ALLOW_READY_WEAPON_KEYBOARD_DEFAULT_VALUE, ALLOW_READY_WEAPON_CONTROLLER_DEFAULT_VALUE };
		bool allowLeftAttack[kTotal]{ ALLOW_LEFT_ATTACK_KEYBOARD_DEFAULT_VALUE, ALLOW_LEFT_ATTACK_CONTROLLER_DEFAULT_VALUE };
		bool allowRightAttack[kTotal]{ ALLOW_RIGHT_ATTACK_KEYBOARD_DEFAULT_VALUE, ALLOW_RIGHT_ATTACK_CONTROLLER_DEFAULT_VALUE };
		bool allowHotkeys[kTotal]{ ALLOW_HOTKEYS_KEYBOARD_DEFAULT_VALUE, ALLOW_HOTKEYS_CONTROLLER_DEFAULT_VALUE };
		bool allowShout[kTotal]{ ALLOW_SHOUT_KEYBOARD_DEFAULT_VALUE, ALLOW_SHOUT_CONTROLLER_DEFAULT_VALUE };

		// Auto-close
		bool autoCloseMenus{ AUTO_CLOSE_MENUS_DEFAULT_VALUE };
		float autoCloseDistance{ AUTO_CLOSE_DISTANCE_DEFAULT_VALUE };
		float autoCloseTolerance{ AUTO_CLOSE_TOLERANCE_DEFAULT_VALUE };

		// Optional LE call-site overrides (0 = auto-discover at runtime)
		std::uintptr_t hookGetDialogueLookAngle{ 0 };
		std::uintptr_t hookStartDialogueForceGreet{ 0 };
		std::uintptr_t hookInitDialogueLookAt{ 0 };
		std::uintptr_t hookIsGamepadEnabled{ 0 };

	private:
		Settings() = default;
		~Settings() = default;
		Settings(const Settings&) = delete;
		Settings& operator=(const Settings&) = delete;
	};

	void LoadSettings(bool a_saveBack = true);
	void SaveSettings();
	void RestoreDefaults();
	bool ReloadSettingsIfModified();
}
