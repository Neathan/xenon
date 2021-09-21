#pragma once

#ifndef XE_NO_LOG
#include <spdlog/spdlog.h>
#endif

namespace xe {

	#ifndef XE_NO_LOG
		#define XE_LOG_TRACE_F(fmt, ...)	spdlog::trace(fmt, __VA_ARGS__)
		#define XE_LOG_TRACE(msg)			spdlog::trace(msg)
		#define XE_LOG_INFO_F(fmt, ...)		spdlog::info(fmt, __VA_ARGS__)
		#define XE_LOG_INFO(msg)			spdlog::info(msg)
		#define XE_LOG_DEBUG_F(fmt, ...)	spdlog::debug(fmt, __VA_ARGS__)
		#define XE_LOG_DEBUG(msg)			spdlog::debug(msg)
		#define XE_LOG_WARN_F(fmt, ...)		spdlog::warn(fmt, __VA_ARGS__)
		#define XE_LOG_WARN(msg)			spdlog::warn(msg)
		#define XE_LOG_ERROR_F(fmt, ...)	spdlog::error(fmt, __VA_ARGS__)
		#define XE_LOG_ERROR(msg)			spdlog::error(msg)
		#define XE_LOG_CRITICAL_F(fmt, ...)	spdlog::critical(fmt, __VA_ARGS__)
		#define XE_LOG_CRITICAL(msg)		spdlog::critical(msg)

		#define XE_SET_LOG_LEVEL(level)		spdlog::set_level(level)
		#define XE_LOG_LEVEL_TRACE			spdlog::level::trace
		#define XE_LOG_LEVEL_DEBUG			spdlog::level::debug
		#define XE_LOG_LEVEL_INFO			spdlog::level::info
		#define XE_LOG_LEVEL_WARN			spdlog::level::warn
		#define XE_LOG_LEVEL_CRITICAL		spdlog::level::critical
	#else
		#define XE_LOG_TRACE_F(fmt, ...)
		#define XE_LOG_TRACE(msg)
		#define XE_LOG_INFO_F(fmt, ...)
		#define XE_LOG_INFO(msg)
		#define XE_LOG_DEBUG_F(fmt, ...)
		#define XE_LOG_DEBUG(msg)
		#define XE_LOG_WARN_F(fmt, ...)
		#define XE_LOG_WARN(msg)
		#define XE_LOG_ERROR_F(fmt, ...)
		#define XE_LOG_ERROR(msg)
		#define XE_LOG_CRITICAL_F(fmt, ...)
		#define XE_LOG_CRITICAL(msg)

		#define XE_SET_LOG_LEVEL(level)
		#define XE_LOG_LEVEL_TRACE
		#define XE_LOG_LEVEL_DEBUG
		#define XE_LOG_LEVEL_INFO
		#define XE_LOG_LEVEL_WARN
		#define XE_LOG_LEVEL_CRITICAL
	#endif

}
