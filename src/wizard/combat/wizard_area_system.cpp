#include "wizard/combat/wizard_area_system.hpp"

#include <algorithm>
#include <cmath>

namespace {
	void appendPosition(std::vector<Position> &positions, const Position &center, int32_t dx, int32_t dy) {
		const int32_t x = static_cast<int32_t>(center.x) + dx;
		const int32_t y = static_cast<int32_t>(center.y) + dy;
		if (x >= 0 && x <= std::numeric_limits<uint16_t>::max() && y >= 0 && y <= std::numeric_limits<uint16_t>::max()) {
			positions.emplace_back(static_cast<uint16_t>(x), static_cast<uint16_t>(y), center.z);
		}
	}

	std::pair<int32_t, int32_t> rotateOffset(int32_t x, int32_t y, Direction direction) {
		switch (direction) {
			case DIRECTION_EAST:
				return { -y, x };
			case DIRECTION_SOUTH:
				return { -x, -y };
			case DIRECTION_WEST:
				return { y, -x };
			default:
				return { x, y };
		}
	}
}

uint16_t WizardAreaSystem::calculateEffectiveSquares(
	const uint16_t magicalPower,
	const uint16_t minSquares,
	const uint16_t maxSquares
) {
	if (maxSquares <= minSquares) {
		return minSquares;
	}

	const auto power = std::clamp<uint16_t>(magicalPower, 1, 100);

	// O potencial máximo da magia somente é alcançado
	// com Poder Mágico 100.
	if (power == 100) {
		return maxSquares;
	}

	const auto availableSquares = maxSquares - minSquares;

	const double powerRatio =
		static_cast<double>(power) / 100.0;

	const auto unlockedSquares =
		static_cast<uint16_t>(
			std::floor(
				static_cast<double>(availableSquares) * powerRatio
			)
		);

	return minSquares + unlockedSquares;
}

std::vector<Position> WizardAreaSystem::resolve(
	const WizardAreaDefinition &area,
	const Position &center,
	const uint16_t magicalPower,
	const Direction direction
) {
	const auto required = calculateEffectiveSquares(magicalPower, area.minSquares, area.maxSquares);
	std::vector<Position> positions;
	positions.reserve(required);
	appendPosition(positions, center, 0, 0);

	for (int32_t radius = 1; positions.size() < required; ++radius) {
		std::vector<std::pair<int32_t, int32_t>> offsets;
		switch (area.pattern) {
			case WizardAreaPattern::NONE:
				return positions;
			case WizardAreaPattern::CROSS:
				offsets = { { 0, -radius }, { radius, 0 }, { 0, radius }, { -radius, 0 } };
				break;
			case WizardAreaPattern::LINE:
				offsets = { { 0, -radius } };
				break;
			case WizardAreaPattern::CONE:
			case WizardAreaPattern::WAVE:
				for (int32_t x = -radius; x <= radius; ++x) {
					offsets.emplace_back(x, -radius);
				}
				break;
			case WizardAreaPattern::RING:
			case WizardAreaPattern::CIRCLE:
			case WizardAreaPattern::CUSTOM:
				for (int32_t y = -radius; y <= radius; ++y) {
					for (int32_t x = -radius; x <= radius; ++x) {
						if (std::max(std::abs(x), std::abs(y)) == radius) {
							offsets.emplace_back(x, y);
						}
					}
				}
				break;
		}

		for (const auto &[x, y] : offsets) {
			const auto [rotatedX, rotatedY] = rotateOffset(x, y, direction);
			appendPosition(positions, center, rotatedX, rotatedY);
			if (positions.size() == required) {
				break;
			}
		}
	}
	return positions;
}
