#include "wizard/progression/wizard_knowledge_types.hpp"

#include "utils/tools.hpp"

const char* wizardKnowledgeSourceName(const WizardKnowledgeSource source) {
	switch (source) {
		case WizardKnowledgeSource::READING: return "READING";
		case WizardKnowledgeSource::EXPLORATION: return "EXPLORATION";
		case WizardKnowledgeSource::EXPERIMENTATION: return "EXPERIMENTATION";
		case WizardKnowledgeSource::ADMIN: return "ADMIN";
		case WizardKnowledgeSource::SYSTEM: return "SYSTEM";
	}
	return "SYSTEM";
}

const char* wizardAcquisitionProfileName(const WizardAcquisitionProfile profile) {
	switch (profile) {
		case WizardAcquisitionProfile::ACADEMIC: return "ACADEMIC";
		case WizardAcquisitionProfile::EXPLORATION: return "EXPLORATION";
		case WizardAcquisitionProfile::HYBRID: return "HYBRID";
		case WizardAcquisitionProfile::SECRET: return "SECRET";
		case WizardAcquisitionProfile::FORBIDDEN: return "FORBIDDEN";
	}
	return "SECRET";
}

bool parseWizardKnowledgeSource(const std::string &value, WizardKnowledgeSource &source) {
	const auto normalized = asUpperCaseString(value);
	if (normalized == "READING") source = WizardKnowledgeSource::READING;
	else if (normalized == "EXPLORATION") source = WizardKnowledgeSource::EXPLORATION;
	else if (normalized == "EXPERIMENTATION") source = WizardKnowledgeSource::EXPERIMENTATION;
	else if (normalized == "ADMIN") source = WizardKnowledgeSource::ADMIN;
	else if (normalized == "SYSTEM") source = WizardKnowledgeSource::SYSTEM;
	else return false;
	return true;
}

bool parseWizardAcquisitionProfile(const std::string &value, WizardAcquisitionProfile &profile) {
	const auto normalized = asUpperCaseString(value);
	if (normalized == "ACADEMIC") profile = WizardAcquisitionProfile::ACADEMIC;
	else if (normalized == "EXPLORATION") profile = WizardAcquisitionProfile::EXPLORATION;
	else if (normalized == "HYBRID") profile = WizardAcquisitionProfile::HYBRID;
	else if (normalized == "SECRET") profile = WizardAcquisitionProfile::SECRET;
	else if (normalized == "FORBIDDEN") profile = WizardAcquisitionProfile::FORBIDDEN;
	else return false;
	return true;
}
