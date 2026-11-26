#pragma once

#include "CommonLibLE.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Verified Skyrim LE 1.9.32.0 (TESV.exe) UI, Input, Menu, and Dialogue definitions.
// All offsets and addresses are cross-verified against libSkyrim, skyrim-classic-re,
// and CommonLibLE's RE_DATABASE for TESV.exe 1.9.32.0.
namespace LE
{
	namespace Addresses
	{
		// Confirmed static vtables & singletons (TESV.exe 1.9.32.0)
		inline constexpr std::uintptr_t kMenuControls_VTable = 0x010E6A60;       // slot 1: ReceiveEvent (0x0087A3D0)
		inline constexpr std::uintptr_t kDialogueMenu_VTable = 0x010E4C9C;       // slot 4: ProcessMessage, slot 5: NextFrame
		inline constexpr std::uintptr_t kMenuTopicManager_Singleton = 0x013105D8;
		inline constexpr std::uintptr_t kUIManager_Singleton = 0x012E35E4;
		inline constexpr std::uintptr_t kUIStringHolder_Singleton = 0x012E35E0;

		// Confirmed static functions (TESV.exe 1.9.32.0)
		inline constexpr std::uintptr_t kMenuManager_IsInMenuMode = 0x00918D90;
		inline constexpr std::uintptr_t kUIManager_AddMessage = 0x00431B00;
		inline constexpr std::uintptr_t kInputEventDispatcher_IsGamepadEnabled = 0x00A68E80;
		inline constexpr std::uintptr_t kPlayerControls_ReceiveMenuModeChange = 0x00772B10;
		inline constexpr std::uintptr_t kPlayerControls_ReceiveMenuOpenClose = 0x00773340;
		inline constexpr std::uintptr_t kActor_SetDialogueWithPlayer = 0x006C0D90; // Actor::Unk_41
		inline constexpr std::uintptr_t kMenuTopicManager_StartDialogue = 0x006758A0;
		inline constexpr std::uintptr_t kMenuTopicManager_GetActorInDialogue = 0x00674450;

		// PlayerCharacter dialogue camera block (noted in skyrim-classic-re MenuTopicManager.h:
		// 0x0073C3DF and 0x0073C736 query MenuTopicManager::GetActorInDialogueWithPlayer and control camera)
		inline constexpr std::uintptr_t kPlayerDialogueCamera_RangeStart = 0x0073C000;
		inline constexpr std::uintptr_t kPlayerDialogueCamera_RangeEnd = 0x0073CC00;
	}

	inline bool FixedStringEquals(const RE::BSFixedString& a_lhs, const RE::BSFixedString& a_rhs) noexcept
	{
		const char* l = a_lhs.c_str();
		const char* r = a_rhs.c_str();
		if (l == r) {
			return true;
		}
		return _stricmp(l, r) == 0;
	}

	inline void CopyFixedString(RE::BSFixedString& a_dst, const RE::BSFixedString& a_src)
	{
		if (a_dst.c_str() != a_src.c_str()) {
			a_dst.set(a_src.c_str());
		}
	}

	inline bool IsInMenuMode() noexcept
	{
		using fn_t = bool(CLLE_CDECL*)();
		return reinterpret_cast<fn_t>(Addresses::kMenuManager_IsInMenuMode)();
	}

	// =========================================================================
	// Input Events (exact TESV.exe 1.9.32.0 layout from libSkyrim/InputEvent.h)
	// =========================================================================
	struct InputEvent
	{
		enum DeviceType : std::uint32_t
		{
			kDevice_Keyboard = 0,
			kDevice_Mouse = 1,
			kDevice_Gamepad = 2
		};

		enum EventType : std::uint32_t
		{
			kEventType_Button = 0,
			kEventType_MouseMove = 1,
			kEventType_Char = 2,
			kEventType_Thumbstick = 3,
			kEventType_DeviceConnect = 4,
			kEventType_Kinect = 5
		};

		void** vtable{};           // 00
		std::uint32_t deviceType{}; // 04
		std::uint32_t eventType{};  // 08
		InputEvent* next{};         // 0C

		[[nodiscard]] bool IsIDEvent() const noexcept
		{
			using fn_t = bool(CLLE_THISCALL*)(const InputEvent*);
			return reinterpret_cast<fn_t>(vtable[1])(this);
		}
	};
	CLLE_STATIC_ASSERT_SIZE(InputEvent, 0x10);
	CLLE_STATIC_ASSERT_OFFSET(InputEvent, next, 0x0C);

	struct IDEvent : public InputEvent
	{
		RE::BSFixedString controlID{}; // 10
	};
	CLLE_STATIC_ASSERT_SIZE(IDEvent, 0x14);
	CLLE_STATIC_ASSERT_OFFSET(IDEvent, controlID, 0x10);

	struct ButtonEvent : public IDEvent
	{
		std::uint32_t keyMask{}; // 14
		float pressure{};        // 18
		float timer{};           // 1C
	};
	CLLE_STATIC_ASSERT_SIZE(ButtonEvent, 0x20);
	CLLE_STATIC_ASSERT_OFFSET(ButtonEvent, keyMask, 0x14);

	struct ThumbstickEvent : public IDEvent
	{
		std::uint32_t keyMask{}; // 14 (0x0B left, 0x0C right)
		float x{};               // 18
		float y{};               // 1C
	};
	CLLE_STATIC_ASSERT_SIZE(ThumbstickEvent, 0x20);
	CLLE_STATIC_ASSERT_OFFSET(ThumbstickEvent, keyMask, 0x14);

	// =========================================================================
	// MovementHandler (libSkyrim/PlayerInputHandler.h)
	// =========================================================================
	struct MovementHandler
	{
		void** vtable{};         // 00
		std::uint32_t enabled{}; // 04 (PlayerInputHandler::enabled)
		bool disabled{};         // 08 (MovementHandler::disabled)
		std::uint8_t pad09[3]{};

		[[nodiscard]] bool IsEnabled() const noexcept
		{
			return enabled != 0 && !disabled;
		}
	};

	// =========================================================================
	// InputMappingManager / ControlMap helpers (libSkyrim/InputMappingManager.h)
	// =========================================================================
	struct InputMappingData
	{
		RE::BSFixedString name{}; // 00
		std::uint16_t buttonID{}; // 04
		std::uint16_t modifier{}; // 06
		std::uint32_t sortIndex{}; // 08
		std::uint32_t flags{};     // 0C
	};
	CLLE_STATIC_ASSERT_SIZE(InputMappingData, 0x10);

	struct InputMappingContext
	{
		RE::tArray<InputMappingData> keyboardMap{}; // 00
		RE::tArray<InputMappingData> mouseMap{};    // 0C
		RE::tArray<InputMappingData> gamepadMap{};  // 18
	};
	CLLE_STATIC_ASSERT_SIZE(InputMappingContext, 0x24);

	inline constexpr std::uint32_t kInvalidKey = 0xFFFFFFFFu;
	inline constexpr std::int32_t kControlState_Movement = 0x401;

	inline bool IsMovementControlsEnabled(const RE::InputManager* a_inputMgr) noexcept
	{
		if (!a_inputMgr) {
			return false;
		}
		const auto controlState = *reinterpret_cast<const std::int32_t*>(
			reinterpret_cast<const std::byte*>(a_inputMgr) + 0x90);
		return (controlState & kControlState_Movement) == kControlState_Movement;
	}

	inline std::uint32_t GetMappedKey(
		const RE::InputManager* a_inputMgr,
		const RE::BSFixedString& a_controlName,
		std::uint32_t a_deviceType,
		std::uint32_t a_contextIdx = RE::InputManager::kContextGameplay) noexcept
	{
		if (!a_inputMgr || a_contextIdx >= RE::InputManager::kContextCount) {
			return kInvalidKey;
		}

		const auto* ctx = reinterpret_cast<const InputMappingContext*>(a_inputMgr->contexts[a_contextIdx]);
		if (!ctx) {
			return kInvalidKey;
		}

		const RE::tArray<InputMappingData>* maps = nullptr;
		if (a_deviceType == InputEvent::kDevice_Keyboard) {
			maps = &ctx->keyboardMap;
		} else if (a_deviceType == InputEvent::kDevice_Mouse) {
			maps = &ctx->mouseMap;
		} else if (a_deviceType == InputEvent::kDevice_Gamepad) {
			maps = &ctx->gamepadMap;
		}

		if (!maps || !maps->data) {
			return kInvalidKey;
		}

		for (std::uint32_t i = 0; i < maps->count; ++i) {
			if (FixedStringEquals(maps->data[i].name, a_controlName)) {
				return static_cast<std::uint32_t>(maps->data[i].buttonID);
			}
		}

		return kInvalidKey;
	}

	// =========================================================================
	// InputStringHolder field accessor (libSkyrim/InputManager.h verified offsets)
	// =========================================================================
	inline const RE::BSFixedString& GetInputStringAt(const RE::InputStringHolder* a_holder, std::size_t a_offset) noexcept
	{
		return *reinterpret_cast<const RE::BSFixedString*>(
			reinterpret_cast<const std::byte*>(a_holder) + a_offset);
	}

	namespace InputStringOffset
	{
		inline constexpr std::size_t kForward = 0x004;
		inline constexpr std::size_t kBack = 0x008;
		inline constexpr std::size_t kStrafeLeft = 0x00C;
		inline constexpr std::size_t kStrafeRight = 0x010;
		inline constexpr std::size_t kMove = 0x014;
		inline constexpr std::size_t kLeftAttack = 0x020;
		inline constexpr std::size_t kRightAttack = 0x024;
		inline constexpr std::size_t kReadyWeapon = 0x034;
		inline constexpr std::size_t kTogglePOV = 0x038;
		inline constexpr std::size_t kJump = 0x03C;
		inline constexpr std::size_t kSprint = 0x044;
		inline constexpr std::size_t kSneak = 0x048;
		inline constexpr std::size_t kShout = 0x04C;
		inline constexpr std::size_t kRun = 0x058;
		inline constexpr std::size_t kToggleRun = 0x05C;
		inline constexpr std::size_t kLeftStick = 0x100;
		inline constexpr std::size_t kHotkey1 = 0x124;
		inline constexpr std::size_t kHotkey2 = 0x128;
		inline constexpr std::size_t kHotkey3 = 0x12C;
		inline constexpr std::size_t kHotkey4 = 0x130;
		inline constexpr std::size_t kHotkey5 = 0x134;
		inline constexpr std::size_t kHotkey6 = 0x138;
		inline constexpr std::size_t kHotkey7 = 0x13C;
		inline constexpr std::size_t kHotkey8 = 0x140;
	}

	// =========================================================================
	// MenuControls (libSkyrim/MenuControls.h)
	// =========================================================================
	struct MenuControls
	{
		std::array<std::byte, 0x40> pad00{}; // 00
		bool nowProcessing{};                // 40
		std::uint8_t pad41{};                // 41
		bool remapMode{};                    // 42
		std::uint8_t pad43{};                // 43
	};
	CLLE_STATIC_ASSERT_OFFSET(MenuControls, remapMode, 0x42);

	// =========================================================================
	// UIMessage, IMenu, DialogueMenu, UIManager, UIStringHolder
	// =========================================================================
	struct UIMessage
	{
		enum Type : std::uint32_t
		{
			kRefresh = 0,
			kShow = 1,
			kPreviouslyKnownAsClose = 2,
			kHide = 3,
			kScaleform = 6,
			kMessage = 7
		};

		RE::BSFixedString name{}; // 00
		Type type{};              // 04
		void* data{};             // 08
		std::uint8_t isPooled{};  // 0C
		std::uint8_t pad0D[3]{};  // 0D
	};
	CLLE_STATIC_ASSERT_SIZE(UIMessage, 0x10);

	struct IMenu
	{
		enum Flag : std::uint32_t
		{
			kPauseGame = 0x00000001,
			kDoNotDeleteOnClose = 0x00000002,
			kUsesCursor = 0x00000004,
			kUpdateUsesCursor = 0x00000400,
			kDontHideCursorWhenTopmost = 0x00080000
		};

		void** vtable{};         // 00
		std::int32_t refCount{}; // 04
		void* view{};            // 08
		std::uint8_t menuDepth{}; // 0C
		std::uint8_t pad0D[3]{};  // 0D
		std::uint32_t flags{};    // 10
		std::uint32_t context{};  // 14
		void* fxDelegate{};       // 18
	};
	CLLE_STATIC_ASSERT_SIZE(IMenu, 0x1C);
	CLLE_STATIC_ASSERT_OFFSET(IMenu, flags, 0x10);

	struct DialogueMenu : public IMenu
	{
		void* menuOpenCloseSinkVTable{}; // 1C
		std::uint32_t unk20{};           // 20
		std::uint32_t unk24{};           // 24
		std::uint32_t unk28{};           // 28
	};
	CLLE_STATIC_ASSERT_SIZE(DialogueMenu, 0x2C);

	struct UIStringHolder
	{
		std::array<std::byte, 0x4C> pad00{};
		RE::BSFixedString dialogueMenu{}; // 4C "Dialogue Menu"

		static UIStringHolder* GetSingleton() noexcept
		{
			return *reinterpret_cast<UIStringHolder**>(Addresses::kUIStringHolder_Singleton);
		}
	};
	CLLE_STATIC_ASSERT_OFFSET(UIStringHolder, dialogueMenu, 0x4C);

	struct UIManager
	{
		static UIManager* GetSingleton() noexcept
		{
			return *reinterpret_cast<UIManager**>(Addresses::kUIManager_Singleton);
		}

		void AddMessage(const RE::BSFixedString& a_menuName, std::uint32_t a_msgID, void* a_data) noexcept
		{
			using fn_t = void(CLLE_THISCALL*)(UIManager*, const RE::BSFixedString&, std::uint32_t, void*);
			reinterpret_cast<fn_t>(Addresses::kUIManager_AddMessage)(this, a_menuName, a_msgID, a_data);
		}
	};

	// =========================================================================
	// MenuTopicManager (skyrim-classic-re/ReverseEngineered/Systems/MenuTopicManager.h)
	// =========================================================================
	struct MenuTopicManager
	{
		std::array<std::byte, 0x38> pad00{};
		RE::RefHandle speaker{};     // 38 (talkingHandle)
		RE::RefHandle lastSpeaker{}; // 3C (handle2)

		static MenuTopicManager* GetSingleton() noexcept
		{
			return *reinterpret_cast<MenuTopicManager**>(Addresses::kMenuTopicManager_Singleton);
		}
	};
	CLLE_STATIC_ASSERT_OFFSET(MenuTopicManager, speaker, 0x38);
	CLLE_STATIC_ASSERT_OFFSET(MenuTopicManager, lastSpeaker, 0x3C);

	// =========================================================================
	// TESObjectREFR bounding box virtual helpers (libSkyrim/TESObjectREFR.h slots 0x73 / 0x74)
	// =========================================================================
	inline RE::NiPoint3 GetBoundMin(const RE::TESObjectREFR* a_ref) noexcept
	{
		RE::NiPoint3 out{};
		if (!a_ref) {
			return out;
		}
		RE::invoke_vfunc<RE::NiPoint3*, RE::NiPoint3*>(a_ref, 0x73, &out);
		return out;
	}

	inline RE::NiPoint3 GetBoundMax(const RE::TESObjectREFR* a_ref) noexcept
	{
		RE::NiPoint3 out{};
		if (!a_ref) {
			return out;
		}
		RE::invoke_vfunc<RE::NiPoint3*, RE::NiPoint3*>(a_ref, 0x74, &out);
		return out;
	}
}
