#pragma once

#include "glad/glad.h"
#include <vector>

class Framebuffer {
	GLuint width, height;

public:
	GLuint framebuffer, framebufferTexture, depthRenderbuffer;
	Framebuffer(GLuint width, GLuint height, GLenum texture_format);

	~Framebuffer();

	void Bind();
	void BindToTexture(GLenum texture_unit);
	std::vector<GLbyte> GetImage();
};

