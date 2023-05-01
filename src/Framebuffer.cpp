#include <Framebuffer.h>

Framebuffer::Framebuffer(GLuint width, GLuint height, GLenum texture_format) {
	this->width = width;
	this->height = height;

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	glGenTextures(1, &framebufferTexture);
	glBindTexture(GL_TEXTURE_2D, framebufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, texture_format, width, height, 0, texture_format, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenRenderbuffers(1, &depthRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);
}

Framebuffer::~Framebuffer() {
	glDeleteTextures(1, &framebufferTexture);
	glDeleteRenderbuffers(1, &depthRenderbuffer);
	glDeleteFramebuffers(1, &framebuffer);
}

void Framebuffer::Bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
}

void Framebuffer::BindToTexture(GLenum texture_unit) {
	glActiveTexture(texture_unit);
	glBindTexture(GL_TEXTURE_2D, framebufferTexture);
}

std::vector<GLbyte> Framebuffer::GetImage() {
	std::vector<GLbyte> bytes;
	bytes.resize(width * height * 3);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes.data());
	return bytes;
}
