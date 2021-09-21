#pragma once

#include <xhash>

namespace xe {

	struct UUID {
		uint32_t value;

		operator uint32_t() { return value; }
		operator const uint32_t() const { return value; }
		UUID();
		UUID(uint32_t uuid) : value(uuid) {}
		UUID(const UUID& other) : value(other.value) {}

		bool isValid() const { return value != 0; }

		static UUID None() { return UUID(0); };
	};

}

namespace std {

	template<>
	struct hash<xe::UUID> {
		std::size_t operator()(const xe::UUID& uuid)  const {
			return hash<uint32_t>()((uint32_t)uuid);
		}
	};

}
