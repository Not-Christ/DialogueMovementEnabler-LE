#include "Hooks.h"
#include "AutoCloseManager.h"
#include "LECompat.h"
#include "Settings.h"

namespace DME
{
	namespace
	{
		using GetDialogueLookAngle_t = float(CLLE_THISCALL*)(RE::PlayerCharacter*);
		GetDialogueLookAngle_t _GetDialogueLookAngle = nullptr;

		using InitDialogueLookAt_t = void(CLLE_THISCALL*)(RE::PlayerCharacter*);
		InitDialogueLookAt_t _InitDialogueLookAt = nullptr;

		using IsGamepadEnabled_t = bool(CLLE_THISCALL*)(RE::InputEventDispatcher*);
		IsGamepadEnabled_t _IsGamepadEnabled = nullptr;

		using StartDialogue_t = bool(CLLE_THISCALL*)(LE::MenuTopicManager*, RE::TESObjectREFR*, bool, void*, bool);
		StartDialogue_t _StartDialogue = nullptr;

		bool s_isNPCInitiatedDialogue = false;

		// =====================================================================
		// Call-site hooks (x86 __fastcall with dummy EDX matches __thiscall ABI)
		// =====================================================================
		float CLLE_FASTCALL GetDialogueLookAngle_Hook(RE::PlayerCharacter* a_player, void* /*edx*/)
		{
			const Settings* settings = Settings::GetSingleton();
			if (settings->unlockCamera || settings->freeLook) {
				return std::numeric_limits<float>::max();
			}
			return _GetDialogueLookAngle(a_player);
		}

		bool CLLE_FASTCALL Actor_SetDialogueWithPlayer_ForceGreet_Hook(
			LE::MenuTopicManager* a_topicManager,
			void* /*edx*/,
			RE::TESObjectREFR* a_speaker,
			bool a_unk1,
			void* a_topicInfo,
			bool a_unk2)
		{
			s_isNPCInitiatedDialogue = true;
			const bool result = _StartDialogue(a_topicManager, a_speaker, a_unk1, a_topicInfo, a_unk2);
			s_isNPCInitiatedDialogue = false;
			return result;
		}

		void CLLE_FASTCALL InitDialogueLookAt_Hook(RE::PlayerCharacter* a_player, void* /*edx*/)
		{
			const Settings* settings = Settings::GetSingleton();
			const bool disableTurn = s_isNPCInitiatedDialogue
				? settings->disableTurnToSpeakerNPCInitiated
				: settings->disableTurnToSpeakerPlayerInitiated;

			if (disableTurn) {
				return;
			}
			_InitDialogueLookAt(a_player);
		}

		bool CLLE_FASTCALL PlayerControls_MenuOpenClose_IsGamepadEnabled_Hook(
			RE::InputEventDispatcher* a_inputDispatcher,
			void* /*edx*/)
		{
			// Decides whether the cursor turns the camera at the screen edges while the dialogue menu is open,
			// which the game only does when no gamepad is enabled.
			if (Settings::GetSingleton()->freeLook) {
				return true;
			}
			return _IsGamepadEnabled(a_inputDispatcher);
		}

		// =====================================================================
		// Runtime rel32 CALL scanner helpers (validates live TESV.exe 1.9.32.0 bytes)
		// =====================================================================
		std::uintptr_t FindRelCallToExactTarget(
			std::uintptr_t a_start,
			std::size_t a_maxBytes,
			std::uintptr_t a_expectedTarget) noexcept
		{
			std::vector<std::uint8_t> buf(a_maxBytes);
			if (!REL::safe_read(a_start, buf.data(), a_maxBytes)) {
				return 0;
			}

			for (std::size_t i = 0; i + 5 <= a_maxBytes; ++i) {
				if (buf[i] != 0xE8) {
					continue;
				}
				std::int32_t disp = 0;
				std::memcpy(&disp, &buf[i + 1], sizeof(disp));
				const std::uintptr_t site = a_start + i;
				const std::uintptr_t target = static_cast<std::uintptr_t>(
					static_cast<std::intptr_t>(site + 5) + static_cast<std::intptr_t>(disp));
				if (target == a_expectedTarget) {
					return site;
				}
			}
			return 0;
		}

		std::uintptr_t FindRelCallToRange(
			std::uintptr_t a_start,
			std::size_t a_maxBytes,
			std::uintptr_t a_targetMin,
			std::uintptr_t a_targetMax,
			std::uintptr_t& a_outTarget) noexcept
		{
			a_outTarget = 0;
			std::vector<std::uint8_t> buf(a_maxBytes);
			if (!REL::safe_read(a_start, buf.data(), a_maxBytes)) {
				return 0;
			}

			for (std::size_t i = 0; i + 5 <= a_maxBytes; ++i) {
				if (buf[i] != 0xE8) {
					continue;
				}
				std::int32_t disp = 0;
				std::memcpy(&disp, &buf[i + 1], sizeof(disp));
				const std::uintptr_t site = a_start + i;
				const std::uintptr_t target = static_cast<std::uintptr_t>(
					static_cast<std::intptr_t>(site + 5) + static_cast<std::intptr_t>(disp));
				if (target >= a_targetMin && target <= a_targetMax) {
					a_outTarget = target;
					return site;
				}
			}
			return 0;
		}

		bool PatchRelCall5(std::uintptr_t a_site, void* a_hook, std::uintptr_t& a_outOriginal) noexcept
		{
			a_outOriginal = 0;
			if (!a_site) {
				return false;
			}

			const REL::DecodedTransfer decoded = REL::decode_transfer(a_site);
			if (!decoded.valid || decoded.kind != REL::TransferKind::call_rel32) {
				return false;
			}

			const auto claim = REL::Trampoline::get().write_call5(a_site, a_hook, decoded.target, true);
			if (claim.result != REL::ClaimResult::installed && claim.result != REL::ClaimResult::already_installed) {
				return false;
			}

			a_outOriginal = claim.original_target ? claim.original_target : decoded.target;
			return true;
		}

		template <class HookFn, class OrigFn>
		bool PatchVTableSlot(std::uintptr_t a_vtable, std::size_t a_slot, HookFn a_hook, OrigFn& a_outOriginal) noexcept
		{
			static_assert(sizeof(HookFn) == sizeof(std::uintptr_t), "HookFn must match pointer size on Win32");
			static_assert(sizeof(OrigFn) == sizeof(std::uintptr_t), "OrigFn must match pointer size on Win32");

			const std::uintptr_t slotAddr = a_vtable + a_slot * sizeof(std::uintptr_t);
			std::uintptr_t orig = 0;
			if (!REL::safe_read(slotAddr, &orig, sizeof(orig))) {
				return false;
			}

			std::uintptr_t hookAddr = 0;
			std::memcpy(&hookAddr, &a_hook, sizeof(hookAddr));

			if (!REL::safe_write(slotAddr, &hookAddr, sizeof(hookAddr))) {
				return false;
			}

			std::memcpy(&a_outOriginal, &orig, sizeof(orig));
			return true;
		}

		// =====================================================================
		// VTable Hook: MenuControls::ReceiveEvent (slot 0x1)
		// =====================================================================
		class MenuControlsEx : public LE::MenuControls
		{
		public:
			using ReceiveEvent_t = std::uint32_t(CLLE_THISCALL*)(MenuControlsEx*, LE::InputEvent**, void*);
			static inline ReceiveEvent_t _ReceiveEvent = nullptr;

			static bool IsMappedToSameButton(
				std::uint32_t a_keyMask,
				std::uint32_t a_deviceType,
				const RE::BSFixedString& a_controlName,
				std::uint32_t a_context = RE::InputManager::kContextGameplay) noexcept
			{
				if (a_deviceType != LE::InputEvent::kDevice_Keyboard &&
					a_deviceType != LE::InputEvent::kDevice_Mouse) {
					return false;
				}

				const RE::InputManager* controlMap = RE::InputManager::GetSingleton();
				const std::uint32_t mapped = LE::GetMappedKey(controlMap, a_controlName, a_deviceType, a_context);
				return mapped != LE::kInvalidKey && a_keyMask == mapped;
			}

			std::uint32_t ReceiveEvent_Hook(LE::InputEvent** a_event, void* a_source)
			{
				RE::MenuManager* ui = RE::MenuManager::GetSingleton();
				LE::UIStringHolder* uiStrings = LE::UIStringHolder::GetSingleton();
				RE::PlayerControls* pc = RE::PlayerControls::GetSingleton();
				RE::InputStringHolder* userEvents = RE::InputStringHolder::GetSingleton();
				RE::InputEventDispatcher* inputDispatcher = RE::InputEventDispatcher::GetSingleton();
				RE::InputManager* controlMap = RE::InputManager::GetSingleton();

				const bool isInMenuMode = LE::IsInMenuMode();

				bool movementControlsEnabled = false;
				if (pc && pc->movementHandler && controlMap) {
					const auto* moveHandler = reinterpret_cast<const LE::MovementHandler*>(pc->movementHandler);
					movementControlsEnabled = moveHandler->IsEnabled() && LE::IsMovementControlsEnabled(controlMap);
				}

				if (a_event && *a_event && !this->remapMode && !isInMenuMode &&
					ui && uiStrings && userEvents && inputDispatcher &&
					ui->IsMenuOpen(uiStrings->dialogueMenu)) {
					const Settings* settings = Settings::GetSingleton();
					const Settings::ControlType controlType = inputDispatcher->IsGamepadEnabled()
						? Settings::ControlType::kController
						: Settings::ControlType::kKeyboardMouse;

					const auto& strForward = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kForward);
					const auto& strBack = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kBack);
					const auto& strStrafeLeft = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kStrafeLeft);
					const auto& strStrafeRight = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kStrafeRight);
					const auto& strMove = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kMove);
					const auto& strLeftStick = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kLeftStick);

					const auto& strRun = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kRun);
					const auto& strToggleRun = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kToggleRun);
					const auto& strJump = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kJump);
					const auto& strSprint = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kSprint);
					const auto& strTogglePOV = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kTogglePOV);
					const auto& strSneak = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kSneak);
					const auto& strReadyWeapon = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kReadyWeapon);
					const auto& strLeftAttack = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kLeftAttack);
					const auto& strRightAttack = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kRightAttack);
					const auto& strShout = LE::GetInputStringAt(userEvents, LE::InputStringOffset::kShout);

					for (LE::InputEvent* evn = *a_event; evn; evn = evn->next) {
						if (!evn->IsIDEvent()) {
							continue;
						}

						auto* idEvent = static_cast<LE::IDEvent*>(evn);

						// Controller left stick -> movement
						if (settings->allowMovement[controlType] &&
							LE::FixedStringEquals(idEvent->controlID, strLeftStick)) {
							LE::CopyFixedString(idEvent->controlID, strMove);
						}

						if (evn->eventType != LE::InputEvent::kEventType_Button) {
							continue;
						}

						auto* btn = static_cast<LE::ButtonEvent*>(evn);
						const std::uint32_t keyMask = btn->keyMask;
						const std::uint32_t devType = evn->deviceType;

						// Movement (WASD)
						if (settings->allowMovement[controlType]) {
							if (IsMappedToSameButton(keyMask, devType, strForward)) {
								LE::CopyFixedString(idEvent->controlID, strForward);
							} else if (IsMappedToSameButton(keyMask, devType, strBack)) {
								LE::CopyFixedString(idEvent->controlID, strBack);
							} else if (IsMappedToSameButton(keyMask, devType, strStrafeLeft)) {
								LE::CopyFixedString(idEvent->controlID, strStrafeLeft);
							} else if (IsMappedToSameButton(keyMask, devType, strStrafeRight)) {
								LE::CopyFixedString(idEvent->controlID, strStrafeRight);
							}
						}

						const auto remapIfAllowed = [&](bool a_setting, const RE::BSFixedString& a_targetEvent) {
							if (a_setting && IsMappedToSameButton(keyMask, devType, a_targetEvent)) {
								if (movementControlsEnabled) {
									LE::CopyFixedString(idEvent->controlID, a_targetEvent);
								} else {
									idEvent->controlID.set("");
								}
							}
						};

						remapIfAllowed(settings->allowRun[controlType], strRun);
						remapIfAllowed(settings->allowToggleRun[controlType], strToggleRun);
						remapIfAllowed(settings->allowJump[controlType], strJump);
						remapIfAllowed(settings->allowSprint[controlType], strSprint);
						remapIfAllowed(settings->allowTogglePOV[controlType], strTogglePOV);
						remapIfAllowed(settings->allowSneak[controlType], strSneak);
						remapIfAllowed(settings->allowReadyWeapon[controlType], strReadyWeapon);
						remapIfAllowed(settings->allowLeftAttack[controlType], strLeftAttack);
						remapIfAllowed(settings->allowRightAttack[controlType], strRightAttack);
						remapIfAllowed(settings->allowShout[controlType], strShout);

						// Hotkeys 1..8
						if (settings->allowHotkeys[controlType]) {
							static constexpr std::size_t kHotkeyOffsets[8] = {
								LE::InputStringOffset::kHotkey1,
								LE::InputStringOffset::kHotkey2,
								LE::InputStringOffset::kHotkey3,
								LE::InputStringOffset::kHotkey4,
								LE::InputStringOffset::kHotkey5,
								LE::InputStringOffset::kHotkey6,
								LE::InputStringOffset::kHotkey7,
								LE::InputStringOffset::kHotkey8
							};
							for (std::size_t hkOffset : kHotkeyOffsets) {
								const auto& strHotkey = LE::GetInputStringAt(userEvents, hkOffset);
								if (IsMappedToSameButton(keyMask, devType, strHotkey)) {
									LE::CopyFixedString(idEvent->controlID, strHotkey);
									break;
								}
							}
						}
					}
				}

				return _ReceiveEvent(this, a_event, a_source);
			}
		};

		// =====================================================================
		// VTable Hooks: DialogueMenu::ProcessMessage (slot 0x4) & NextFrame (slot 0x5)
		// =====================================================================
		class DialogueMenuEx : public LE::DialogueMenu
		{
		public:
			using ProcessMessage_t = std::uint32_t(CLLE_THISCALL*)(DialogueMenuEx*, LE::UIMessage*);
			static inline ProcessMessage_t _ProcessMessage = nullptr;

			using NextFrame_t = void(CLLE_THISCALL*)(DialogueMenuEx*, float, std::uint32_t);
			static inline NextFrame_t _NextFrame = nullptr;

			std::uint32_t ProcessMessage_Hook(LE::UIMessage* a_message)
			{
				if (a_message && a_message->type == LE::UIMessage::kShow) {
					// Live-reload INI if user edited it or updated via SkyUI MCM
					ReloadSettingsIfModified();

					// Disable cursor when in free look mode
					if (Settings::GetSingleton()->freeLook) {
						this->flags &= ~(
							LE::IMenu::kUsesCursor |
							LE::IMenu::kUpdateUsesCursor |
							LE::IMenu::kDontHideCursorWhenTopmost);
					}

					RE::TESObjectREFR* target = nullptr;
					if (LE::MenuTopicManager* topicManager = LE::MenuTopicManager::GetSingleton()) {
						if (topicManager->speaker != 0) {
							RE::LookupReferenceByHandle(topicManager->speaker, target);
						}
						if (!target && topicManager->lastSpeaker != 0) {
							RE::LookupReferenceByHandle(topicManager->lastSpeaker, target);
						}
					}

					bool sameWorldSpace = false;
					if (target) {
						if (RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton()) {
							RE::TESObjectCELL* playerCell = player->parentCell;
							RE::TESObjectCELL* targetCell = target->parentCell;

							if (playerCell && targetCell) {
								if (playerCell->IsInterior() && targetCell->IsInterior()) {
									sameWorldSpace = (playerCell == targetCell);
								} else if (!playerCell->IsInterior() && !targetCell->IsInterior()) {
									RE::TESWorldSpace* playerWs = player->GetWorldspace();
									RE::TESWorldSpace* targetWs = target->GetWorldspace();
									sameWorldSpace = (playerWs && targetWs && playerWs == targetWs);
								}
							}
						}
					}

					AutoCloseManager::GetSingleton()->InitAutoClose(sameWorldSpace ? target : nullptr);
				}

				return _ProcessMessage(this, a_message);
			}

			void NextFrame_Hook(float a_interval, std::uint32_t a_currentTime)
			{
				AutoCloseManager::GetSingleton()->CheckAutoClose();
				_NextFrame(this, a_interval, a_currentTime);
			}
		};
	}

	void InstallHooks()
	{
		const Settings* settings = Settings::GetSingleton();

		// 1. VTable hooks (confirmed static addresses for TESV.exe 1.9.32.0)
		if (PatchVTableSlot(LE::Addresses::kMenuControls_VTable, 0x1, &MenuControlsEx::ReceiveEvent_Hook, MenuControlsEx::_ReceiveEvent)) {
			SKSE::log::Write("[HOOK OK] MenuControls::ReceiveEvent (vtbl 0x%08X[1])", static_cast<unsigned>(LE::Addresses::kMenuControls_VTable));
		} else {
			SKSE::log::Write("[HOOK FAIL] MenuControls::ReceiveEvent");
		}

		if (PatchVTableSlot(LE::Addresses::kDialogueMenu_VTable, 0x4, &DialogueMenuEx::ProcessMessage_Hook, DialogueMenuEx::_ProcessMessage)) {
			SKSE::log::Write("[HOOK OK] DialogueMenu::ProcessMessage (vtbl 0x%08X[4])", static_cast<unsigned>(LE::Addresses::kDialogueMenu_VTable));
		} else {
			SKSE::log::Write("[HOOK FAIL] DialogueMenu::ProcessMessage");
		}

		if (PatchVTableSlot(LE::Addresses::kDialogueMenu_VTable, 0x5, &DialogueMenuEx::NextFrame_Hook, DialogueMenuEx::_NextFrame)) {
			SKSE::log::Write("[HOOK OK] DialogueMenu::NextFrame (vtbl 0x%08X[5])", static_cast<unsigned>(LE::Addresses::kDialogueMenu_VTable));
		} else {
			SKSE::log::Write("[HOOK FAIL] DialogueMenu::NextFrame");
		}

		// 2. Call-site hook: PlayerControls::ReceiveEvent(MenuOpenCloseEvent*) -> IsGamepadEnabled (FreeLook)
		{
			const std::uintptr_t site = settings->hookIsGamepadEnabled
				? settings->hookIsGamepadEnabled
				: FindRelCallToExactTarget(
					  LE::Addresses::kPlayerControls_ReceiveMenuOpenClose,
					  0x120,
					  LE::Addresses::kInputEventDispatcher_IsGamepadEnabled);
			std::uintptr_t orig = 0;
			if (PatchRelCall5(site, reinterpret_cast<void*>(&PlayerControls_MenuOpenClose_IsGamepadEnabled_Hook), orig)) {
				_IsGamepadEnabled = reinterpret_cast<IsGamepadEnabled_t>(orig);
				SKSE::log::Write("[HOOK OK] PlayerControls::IsGamepadEnabled call site=0x%08X -> orig=0x%08X", static_cast<unsigned>(site), static_cast<unsigned>(orig));
			} else {
				SKSE::log::Write("[HOOK SKIP] PlayerControls::IsGamepadEnabled call site not found (FreeLook edge-turn bypass disabled; set [HOOKS] uIsGamepadEnabledCallSite to override).");
			}
		}

		// 3. Call-site hook: Actor::SetDialogueWithPlayer (ForceGreet) -> MenuTopicManager::StartDialogue
		{
			const std::uintptr_t site = settings->hookStartDialogueForceGreet
				? settings->hookStartDialogueForceGreet
				: FindRelCallToExactTarget(
					  LE::Addresses::kActor_SetDialogueWithPlayer,
					  0x400,
					  LE::Addresses::kMenuTopicManager_StartDialogue);
			std::uintptr_t orig = 0;
			if (PatchRelCall5(site, reinterpret_cast<void*>(&Actor_SetDialogueWithPlayer_ForceGreet_Hook), orig)) {
				_StartDialogue = reinterpret_cast<StartDialogue_t>(orig);
				SKSE::log::Write("[HOOK OK] Actor::SetDialogueWithPlayer ForceGreet call site=0x%08X -> orig=0x%08X", static_cast<unsigned>(site), static_cast<unsigned>(orig));
			} else {
				SKSE::log::Write("[HOOK SKIP] Actor::SetDialogueWithPlayer ForceGreet call site not found (set [HOOKS] uStartDialogueForceGreetCallSite to override).");
			}
		}

		// 4. Call-site hook: MenuTopicManager::StartDialogue -> PlayerCharacter::InitDialogueLookAt
		{
			std::uintptr_t discoveredTarget = 0;
			const std::uintptr_t site = settings->hookInitDialogueLookAt
				? settings->hookInitDialogueLookAt
				: FindRelCallToRange(
					  LE::Addresses::kMenuTopicManager_StartDialogue,
					  0x450,
					  LE::Addresses::kPlayerDialogueCamera_RangeStart,
					  LE::Addresses::kPlayerDialogueCamera_RangeEnd,
					  discoveredTarget);
			std::uintptr_t orig = 0;
			if (PatchRelCall5(site, reinterpret_cast<void*>(&InitDialogueLookAt_Hook), orig)) {
				_InitDialogueLookAt = reinterpret_cast<InitDialogueLookAt_t>(orig);
				SKSE::log::Write("[HOOK OK] MenuTopicManager::InitDialogueLookAt call site=0x%08X -> orig=0x%08X", static_cast<unsigned>(site), static_cast<unsigned>(orig));
			} else {
				SKSE::log::Write("[HOOK SKIP] InitDialogueLookAt call site not found (TurnToSpeaker disable skipped; set [HOOKS] uInitDialogueLookAtCallSite to override).");
			}
		}

		// 5. Call-site hook: PlayerControls -> PlayerCharacter::GetDialogueLookAngle (UnlockCamera)
		{
			std::uintptr_t discoveredTarget = 0;
			const std::size_t scanSize = static_cast<std::size_t>(
				LE::Addresses::kPlayerControls_ReceiveMenuOpenClose - LE::Addresses::kPlayerControls_ReceiveMenuModeChange);
			const std::uintptr_t site = settings->hookGetDialogueLookAngle
				? settings->hookGetDialogueLookAngle
				: FindRelCallToRange(
					  LE::Addresses::kPlayerControls_ReceiveMenuModeChange,
					  scanSize,
					  LE::Addresses::kPlayerDialogueCamera_RangeStart,
					  LE::Addresses::kPlayerDialogueCamera_RangeEnd,
					  discoveredTarget);
			std::uintptr_t orig = 0;
			if (PatchRelCall5(site, reinterpret_cast<void*>(&GetDialogueLookAngle_Hook), orig)) {
				_GetDialogueLookAngle = reinterpret_cast<GetDialogueLookAngle_t>(orig);
				SKSE::log::Write("[HOOK OK] PlayerControls::GetDialogueLookAngle call site=0x%08X -> orig=0x%08X", static_cast<unsigned>(site), static_cast<unsigned>(orig));
			} else {
				SKSE::log::Write("[HOOK SKIP] GetDialogueLookAngle call site not found (UnlockCamera skipped; set [HOOKS] uGetDialogueLookAngleCallSite to override).");
			}
		}
	}
}
