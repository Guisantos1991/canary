#pragma once

#include <cstdint>
#include <string>

enum class WizardKnowledgeSource : uint8_t {
	READING = 0,
	EXPLORATION = 1,
	EXPERIMENTATION = 2,
	ADMIN = 3,
	SYSTEM = 4,
};

enum class WizardAcquisitionProfile : uint8_t {
	ACADEMIC,
	EXPLORATION,
	HYBRID,
	SECRET,
	FORBIDDEN,
};

using WizardKnowledgeSourceMask = uint16_t;

[[nodiscard]] constexpr bool wizardIsValidKnowledgeSource(const WizardKnowledgeSource source) {
	return static_cast<uint8_t>(source) <= static_cast<uint8_t>(WizardKnowledgeSource::SYSTEM);
}

[[nodiscard]] constexpr WizardKnowledgeSourceMask wizardKnowledgeSourceBit(const WizardKnowledgeSource source) {
	return wizardIsValidKnowledgeSource(source) ? static_cast<WizardKnowledgeSourceMask>(1U << static_cast<uint8_t>(source)) : 0;
}

[[nodiscard]] constexpr bool wizardHasKnowledgeSource(const WizardKnowledgeSourceMask mask, const WizardKnowledgeSource source) {
	return (mask & wizardKnowledgeSourceBit(source)) != 0;
}

[[nodiscard]] const char* wizardKnowledgeSourceName(WizardKnowledgeSource source);
[[nodiscard]] const char* wizardAcquisitionProfileName(WizardAcquisitionProfile profile);
[[nodiscard]] bool parseWizardKnowledgeSource(const std::string &value, WizardKnowledgeSource &source);
[[nodiscard]] bool parseWizardAcquisitionProfile(const std::string &value, WizardAcquisitionProfile &profile);
