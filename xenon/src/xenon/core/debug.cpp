#include "debug.h"

#include "xenon/core/log.h"

namespace xe {

	void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
		// ignore non-significant error/warning codes
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204 || id == 131140) { // 131140 - Blend
			// XE_LOG_TRACE("---------------");
			// XE_LOG_TRACE_F("Trace message ({}): {}", id, message);
			return;
		}

		XE_LOG_DEBUG("---------------");
		XE_LOG_DEBUG_F("Debug message ({}): {}", id, message);

		switch (source) {
		case GL_DEBUG_SOURCE_API:             XE_LOG_DEBUG("Source: API"); break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   XE_LOG_DEBUG("Source: Window System"); break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: XE_LOG_DEBUG("Source: Shader Compiler"); break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     XE_LOG_DEBUG("Source: Third Party"); break;
		case GL_DEBUG_SOURCE_APPLICATION:     XE_LOG_DEBUG("Source: Application"); break;
		case GL_DEBUG_SOURCE_OTHER:           XE_LOG_DEBUG("Source: Other"); break;
		}

		switch (type) {
		case GL_DEBUG_TYPE_ERROR:               XE_LOG_DEBUG("Type: Error"); break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: XE_LOG_DEBUG("Type: Deprecated Behaviour"); break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  XE_LOG_DEBUG("Type: Undefined Behaviour"); break;
		case GL_DEBUG_TYPE_PORTABILITY:         XE_LOG_DEBUG("Type: Portability"); break;
		case GL_DEBUG_TYPE_PERFORMANCE:         XE_LOG_DEBUG("Type: Performance"); break;
		case GL_DEBUG_TYPE_MARKER:              XE_LOG_DEBUG("Type: Marker"); break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          XE_LOG_DEBUG("Type: Push Group"); break;
		case GL_DEBUG_TYPE_POP_GROUP:           XE_LOG_DEBUG("Type: Pop Group"); break;
		case GL_DEBUG_TYPE_OTHER:               XE_LOG_DEBUG("Type: Other"); break;
		}

		switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:         XE_LOG_DEBUG("Severity: high"); break;
		case GL_DEBUG_SEVERITY_MEDIUM:       XE_LOG_DEBUG("Severity: medium"); break;
		case GL_DEBUG_SEVERITY_LOW:          XE_LOG_DEBUG("Severity: low"); break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: XE_LOG_DEBUG("Severity: notification"); break;
		}

		XE_LOG_DEBUG("///////////////");
	}

	void installDebugCallback(GLFWwindow* window) {
		int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
		if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(glDebugOutput, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		}
		else {
			XE_LOG_ERROR("DEBUG: Failed to initialize OpenGL Debug callback because debug window hint is not set");
		}
	}

}
