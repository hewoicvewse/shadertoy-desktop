#pragma once
#include "glad/glad.h"
#include <glm/glm.hpp>
#include <string>
#include <map>
#include <iostream>

class Shader {
	GLuint id;
	bool errored;
	std::map<std::string, GLuint> uniform_location_cache;

public:
	Shader(const std::string &vert, const std::string &frag);
	Shader();
	~Shader();

	void Bind();

	inline GLuint GetOpenGLObject() { return id; }

#define cache_check if (uniform_location_cache.find(name) == uniform_location_cache.end()) uniform_location_cache.insert_or_assign(name, glGetUniformLocation(id, name))
	inline void SetUniform1i(const char *name, GLint value) { cache_check; glUniform1i(uniform_location_cache[name], value); }
	inline void SetUniform1f(const char *name, float value) { cache_check; glUniform1f(uniform_location_cache[name], value); }
	inline void SetUniform3f(const char *name, glm::vec3 value) { cache_check; glUniform3fv(uniform_location_cache[name], 1, &value[0]); }
	inline void SetUniform4f(const char *name, glm::vec4 value) { cache_check; glUniform4fv(uniform_location_cache[name], 1, &value[0]); }
	inline void SetUniformMatrix4f(const char *name, glm::mat4 value) { cache_check; glUniformMatrix4fv(uniform_location_cache[name], 1, GL_FALSE, &value[0][0]); }
#undef cache_check
};

