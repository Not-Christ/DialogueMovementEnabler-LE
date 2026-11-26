#pragma once

#include <cstddef>
#include <string_view>

namespace Version
{
	inline constexpr std::size_t MAJOR = 2;
	inline constexpr std::size_t MINOR = 4;
	inline constexpr std::size_t PATCH = 0;

	inline constexpr std::string_view STRING = "2.4.0";
	inline constexpr std::string_view NAME = "DialogueMovementEnabler";
	inline constexpr std::string_view FORMATTED_NAME = "Dialogue Movement Enabler LE";
	inline constexpr std::string_view AUTHOR = "Vermunds (LE Backport)";
}
