#pragma once

#ifndef XE_NO_ASSERT
#include <cassert>
#endif

namespace xe {

	#ifndef XE_NO_ASSERT
	#define XE_ASSERT(x) assert(x)
	#else
	#define XE_ASSERT(x) 
	#endif
}
