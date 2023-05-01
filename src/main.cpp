#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <string>
#include <vector>
#include <iostream>
#include <Shader.h>
#include <Framebuffer.h>
#include <lodepng.h>
#include <stb_image.h>

const std::string vertex_shader = R"GLSL(#version 330

layout(location = 0) in vec2 vertex_pos;

out vec2 _internal_fragPos;
 
void main() {
    gl_Position = vec4(vertex_pos, 0.0, 1.0);
    _internal_fragPos = vertex_pos;
})GLSL";

const std::string default_fragment_shader = R"GLSL(#version 330

in vec2 _internal_fragPos;

uniform float iTime;
uniform float iTimeDelta;
uniform int iFrame;
uniform vec3 iResolution;
uniform vec4 iMouse;
uniform vec4 iDate;
uniform float iSampleRate;
uniform sampler2D _internal_pausedFrame;

layout(location = 0) out vec4 frag_colour;

void mainImage(out vec4 fragColor, in vec2 fragPos);

void main() {
    vec4 col;
    mainImage(col, (_internal_fragPos * 0.5 + 0.5) * iResolution.xy);
    frag_colour = col;
}

)GLSL";

const std::string fragment_shader_paused = R"G(
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    fragColor = texture(_internal_pausedFrame, _internal_fragPos * 0.5 + 0.5);
})G";

std::string fragment_shader;

static const GLfloat vertex_buffer[] = {
    -1.0, -1.0,
    -1.0,  1.0,
     1.0, -1.0,
     1.0,  1.0    
};

static const GLint index_buffer[] = {
    0, 1, 2,
    1, 2, 3
};

struct Channel {
    std::string path;
    int type;
    void *data;
    unsigned int meta[4]; // opengl object or whatever else is there
};

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "Shadertoy for Desktop", nullptr, nullptr);

    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    char *buffer = new char[1048576];
    strcpy(buffer, "void mainImage(out vec4 fragColor, in vec2 fragCoord) {}");
    Shader *shader = new Shader(vertex_shader, default_fragment_shader + buffer), paused(vertex_shader, default_fragment_shader + fragment_shader_paused);

    GLuint vtxbuf, idxbuf;
    glGenBuffers(1, &vtxbuf);
    glGenBuffers(1, &idxbuf);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vtxbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer), vertex_buffer, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxbuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer), index_buffer, GL_STATIC_DRAW);

    int framecount = 0, display_w, display_h;
    GLuint paused_texture, framebuffer;
    float time, prevtime, deltatime;
    bool pause = false, windowopen = true, fullscreen = false, pause_prevframe = false, ichannelwindowopen = false, cursoronwindow = false;
    bool frameunpause = false;
    float actual_time = 0.0f;
    Channel channels[4];
    glm::vec4 mouse(0, 0, -1, 0);

    glfwGetFramebufferSize(window, &display_w, &display_h);

    Framebuffer fb(display_w, display_h, GL_RGBA);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glfwSwapInterval(1);
    time = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        glClear(GL_COLOR_BUFFER_BIT);

        deltatime = glfwGetTime() - time;
        time = glfwGetTime();

        glBindBuffer(GL_ARRAY_BUFFER, vtxbuf);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxbuf);
        if (!ImGui::GetIO().WantCaptureMouse && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)) {
            double x = 0, y = 0;
            glfwGetCursorPos(window, &x, &y);
            y = display_h - y;
            if (mouse.z < 0) {
                mouse.z = x;
                mouse.w = y;
            }
            mouse.x = x;
            mouse.y = y;
            frameunpause = true;
        } else if (mouse.z >= 0) {
            mouse.z = -abs(mouse.z);
            mouse.w = -abs(mouse.w);
        }

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        tm *nowtm = localtime(&time);
        if (!pause || frameunpause) {
            fb.Bind();
            if (!frameunpause) {
                actual_time += deltatime;
                prevtime = deltatime;
                framecount++;
            }
            /*
            Shader Inputs

            uniform vec3      iResolution;           // viewport resolution (in pixels) (done)
            uniform float     iTime;                 // shader playback time (in seconds) (done)
            uniform float     iTimeDelta;            // render time (in seconds) (done)
            uniform int       iFrame;                // shader playback frame (done)
            uniform float     iChannelTime[4];       // channel playback time (in seconds) (probably font implement)
            uniform vec3      iChannelResolution[4]; // channel resolution (in pixels) (probably wont implement)
            uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click (done)
            uniform samplerXX iChannel0..3;          // input channel. XX = 2D/Cube (probably wont implement)
            uniform vec4      iDate;                 // (year, month, day, time in seconds) (done)
            uniform float     iSampleRate;           // sample rate of audio (done) (useless)
            */
            shader->Bind();
            shader->SetUniform1i("iFrame", framecount);
            shader->SetUniform3f("iResolution", glm::vec3(display_w, display_h, 0));
            shader->SetUniform1f("iTimeDelta", deltatime);
            shader->SetUniform1f("iTime", actual_time);
            shader->SetUniform4f("iMouse", mouse);
            shader->SetUniform4f("iDate", glm::vec4(nowtm->tm_year, nowtm->tm_mon+1, nowtm->tm_mday, nowtm->tm_hour * 3600 + nowtm->tm_min * 60 + nowtm->tm_sec));
            shader->SetUniform1f("iSampleRate", 44100.0f);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		    frameunpause = false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        paused.Bind();
        fb.BindToTexture(GL_TEXTURE0);
        paused.SetUniform1i("_internal_pausedFrame", 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (windowopen) {
            ImGui::Begin(" ");
                ImGui::InputTextMultiline("Shader", buffer, 1048576);

                if (ImGui::Button("Run")) {
                    delete shader;
                    shader = new Shader(vertex_shader, default_fragment_shader + buffer);
                    frameunpause = true;
                }

                if (ImGui::Button("Reset Time")) {
                    actual_time = 0.0f;
                    frameunpause = true;
                }

                if (ImGui::Button(pause ? "Resume" : "Pause")) {
                    pause = !pause;
                    pause_prevframe = pause;
                    prevtime = deltatime;
                }

                if (ImGui::Button("Close")) {
                    windowopen = false;
                }

                if (ImGui::Button("Screenshot")) {
                    auto image_buffer = fb.GetImage();
                    std::string filename = "screenshot_";
                    
                    filename += std::to_string(nowtm->tm_year + 1900) + "_";
                    filename += std::to_string(nowtm->tm_mon + 1) + "_";
                    filename += std::to_string(nowtm->tm_mday) + "_";
                    filename += std::to_string(nowtm->tm_hour) + "_";
                    filename += std::to_string(nowtm->tm_min) + "_";
                    filename += std::to_string(nowtm->tm_sec) + ".png";
                    unsigned char *flipped_buffer = new unsigned char[image_buffer.size()];
                    for (unsigned y = 0; y < display_h; ++y) memcpy(&flipped_buffer[y * display_w * 3], &image_buffer[(display_h - y - 1) * display_w * 3], display_w * 3);
                    lodepng_encode24_file(filename.c_str(), flipped_buffer, display_w, display_h);
                    delete []flipped_buffer;
                }

                if (ImGui::Button("Open channels window")) {
                    ichannelwindowopen = true;
                }

                ImGui::Text("Time: %.3f", actual_time);
                ImGui::Text("Resolution: %dx%d", display_w, display_h);
                ImGui::Text("%.3fFPS", 1.0 / prevtime);
            ImGui::End();
        }

        if (ichannelwindowopen) {
            ImGui::Begin("Channels");
                if (ImGui::Button("Close")) {
                    ichannelwindowopen = false;
                }
                for (unsigned i = 0; i < 4; ++i) {
                    //ImGui::BeginChild(i+1);
                        ImGui::LabelText(channels[i].path.c_str(), "iChannel%d:", i);
                        ImGui::Button("Open");
                        ImGui::Button("Reset");
                    //ImGui::EndChild();
                }
            ImGui::End();
        }

        if (glfwGetKey(window, GLFW_KEY_O)) {
            windowopen = true;            
        }

        if (glfwGetKey(window, GLFW_KEY_SPACE)) {
            if (pause_prevframe == pause) {
                pause = !pause;
                pause_prevframe = pause;
                prevtime = deltatime;
            }
        } else {
            pause_prevframe = !pause_prevframe;
        }

        if (glfwGetKey(window, GLFW_KEY_R)) {
            actual_time = 0.0f;
            frameunpause = true;
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    delete buffer;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
