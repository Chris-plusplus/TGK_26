#pragma once

struct Susceptible {
	static constexpr bool flagComponent = true;
};

struct Infected {
	u32 timer = 0;
};

struct Removed {
	static constexpr bool flagComponent = true;
};