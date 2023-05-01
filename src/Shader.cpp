#include <Shader.h>
#include <iostream>

Shader::Shader(const std::string &vert, const std::string &frag) {
	GLuint vert_program, frag_program;
	GLint status = 0;
	const char *strings[] = { "" };
	errored = false;
	uniform_location_cache = {};

	vert_program = glCreateShader(GL_VERTEX_SHADER);
	strings[0] = vert.c_str();
	glShaderSource(vert_program, 1, strings, NULL);
	glCompileShader(vert_program);
	glGetShaderiv(vert_program, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint log_size = 0;
		char *buffer;
		glGetShaderiv(vert_program, GL_INFO_LOG_LENGTH, &log_size);
		buffer = new char[log_size];
		glGetShaderInfoLog(vert_program, log_size, &log_size, buffer);
		std::cout << "vertex shader error:" << std::endl;
		std::cout << buffer << std::endl;
		glDeleteShader(vert_program);
		errored = true;
		delete []buffer;
		return;
	}

	frag_program = glCreateShader(GL_FRAGMENT_SHADER);
	strings[0] = frag.c_str();
	glShaderSource(frag_program, 1, strings, NULL);
	glCompileShader(frag_program);
	glGetShaderiv(frag_program, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint log_size = 0;
		char *buffer;
		glGetShaderiv(frag_program, GL_INFO_LOG_LENGTH, &log_size);
		buffer = new char[log_size];
		glGetShaderInfoLog(frag_program, log_size, &log_size, buffer);
		std::cout << "fragment shader error:" << std::endl;
		std::cout << buffer << std::endl;
		glDeleteShader(vert_program);
		glDeleteShader(frag_program);
		errored = true;
		delete []buffer;
		return;
	}

	id = glCreateProgram();
	glAttachShader(id, vert_program);
	glAttachShader(id, frag_program);
	glLinkProgram(id);
	glGetProgramiv(id, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLint log_size = 0;
		char *buffer;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_size);
		buffer = new char[log_size];
		glGetProgramInfoLog(id, log_size, &log_size, buffer);
		std::cout << "link error:" << std::endl;
		std::cout << buffer << std::endl;
		glDetachShader(id, vert_program);
		glDetachShader(id, frag_program);
		glDeleteProgram(id);
		glDeleteShader(vert_program);
		glDeleteShader(frag_program);
		errored = true;
		delete []buffer;
		return;
	}

	glDetachShader(id, vert_program);
	glDetachShader(id, frag_program);
	glDeleteShader(vert_program);
	glDeleteShader(frag_program);
}

Shader::Shader() { errored = true; id = 0; }

Shader::~Shader() {
	glUseProgram(0);
	glDeleteProgram(id);
}

void Shader::Bind() {
	if (!errored) {
		glUseProgram(id);
	}
}
