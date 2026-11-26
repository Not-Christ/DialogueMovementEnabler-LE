#include "AutoCloseManager.h"
#include "LECompat.h"
#include "Settings.h"

namespace DME
{
	void AutoCloseManager::CheckAutoClose()
	{
		const Settings* settings = Settings::GetSingleton();
		if (!settings->autoCloseMenus || _data.target == 0) {
			return;
		}

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		RE::TESObjectREFR* target = nullptr;
		if (!RE::LookupReferenceByHandle(_data.target, target) || !target) {
			return;
		}

		const float maxDistance = settings->autoCloseDistance;
		const float tolerance = settings->autoCloseTolerance;

		const float currentDistance = GetBBDistance(player, target);
		const bool tooFarOnOpen = _data.initialDistance > maxDistance;

		if (!tooFarOnOpen && currentDistance > maxDistance) {
			// Normal case
			CloseMenu("Target moved out of range.");
			return;
		} else if (tooFarOnOpen) {
			// Target was opened when it was already too far; close only if distance increases past tolerance
			if (currentDistance > maxDistance && currentDistance > (_data.minDistance + tolerance)) {
				CloseMenu("Target is too far and the distance has increased further.");
				return;
			}
		}

		if (currentDistance < _data.minDistance) {
			_data.minDistance = currentDistance;
		}
	}

	void AutoCloseManager::InitAutoClose(RE::TESObjectREFR* a_ref)
	{
		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
		_data = AutoCloseData{};

		if (a_ref) {
			_data.target = RE::CreateRefHandle(a_ref);
		}
		_data.initialDistance = (player && a_ref) ? GetBBDistance(player, a_ref) : 0.0f;
		_data.minDistance = _data.initialDistance;

		_data.PrintDebugInfo();
	}

	AutoCloseManager* AutoCloseManager::GetSingleton()
	{
		static AutoCloseManager singleton;
		return &singleton;
	}

	void AutoCloseManager::CloseMenu(const char* a_reason)
	{
		SKSE::log::Write("Closing Dialogue Menu: %s", a_reason ? a_reason : "");

		LE::UIManager* uiManager = LE::UIManager::GetSingleton();
		LE::UIStringHolder* uiStrings = LE::UIStringHolder::GetSingleton();
		if (uiManager && uiStrings) {
			uiManager->AddMessage(uiStrings->dialogueMenu, LE::UIMessage::kHide, nullptr);
		}
	}

	float AutoCloseManager::GetBBDistance(const RE::TESObjectREFR* a_refA, const RE::TESObjectREFR* a_refB)
	{
		if (!a_refA || !a_refB) {
			return 0.0f;
		}

		const RE::NiPoint3 posA = a_refA->position;
		const RE::NiPoint3 posB = a_refB->position;

		const RE::NiPoint3 boundMinA = LE::GetBoundMin(a_refA);
		const RE::NiPoint3 boundMaxA = LE::GetBoundMax(a_refA);
		const RE::NiPoint3 boundMinB = LE::GetBoundMin(a_refB);
		const RE::NiPoint3 boundMaxB = LE::GetBoundMax(a_refB);

		const RE::NiPoint3 aMinA{ boundMinA.x + posA.x, boundMinA.y + posA.y, boundMinA.z + posA.z };
		const RE::NiPoint3 aMaxA{ boundMaxA.x + posA.x, boundMaxA.y + posA.y, boundMaxA.z + posA.z };
		const RE::NiPoint3 aMinB{ boundMinB.x + posB.x, boundMinB.y + posB.y, boundMinB.z + posB.z };
		const RE::NiPoint3 aMaxB{ boundMaxB.x + posB.x, boundMaxB.y + posB.y, boundMaxB.z + posB.z };

		const auto axisDist = [](float minA, float maxA, float minB, float maxB) noexcept -> float {
			if (maxA < minB) {
				return minB - maxA;  // A is left/below/behind B
			}
			if (maxB < maxA && maxB < minA) {
				return minA - maxB;  // B is left/below/behind A
			}
			return 0.0f;  // Overlapping on this axis
		};

		const float dx = axisDist(aMinA.x, aMaxA.x, aMinB.x, aMaxB.x);
		const float dy = axisDist(aMinA.y, aMaxA.y, aMinB.y, aMaxB.y);
		const float dz = axisDist(aMinA.z, aMaxA.z, aMinB.z, aMaxB.z);

		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}

	std::string AutoCloseManager::GetRefDebugString(RE::TESObjectREFR* a_ref)
	{
		if (!a_ref) {
			return "NULL";
		}

		const char* name = a_ref->GetReferenceName();
		char buf[256]{};
		std::snprintf(
			buf,
			sizeof(buf),
			"\"%s\" [type=%02X id=%08X]",
			name ? name : "",
			static_cast<unsigned>(a_ref->formType),
			static_cast<unsigned>(a_ref->formID));
		return std::string(buf);
	}

	void AutoCloseManager::AutoCloseData::PrintDebugInfo() const
	{
		RE::TESObjectREFR* ref = nullptr;
		if (target != 0) {
			RE::LookupReferenceByHandle(target, ref);
		}
		const std::string refStr = GetRefDebugString(ref);
		SKSE::log::Write("Auto-close init | Target: %s | Initial distance: %.2f", refStr.c_str(), initialDistance);
	}
}
