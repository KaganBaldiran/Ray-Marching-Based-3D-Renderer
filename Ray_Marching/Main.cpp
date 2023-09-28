#include <glew.h>
#include <raylib.h>
#include <iostream>
#include "Shader.h"
#include "Log.h"
#include "VectorMath.h"
#include "../include/glm/gtc/type_ptr.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"
#include <memory>
#include <string>
#include <time.h>


int main()
{
	float width = 900;
	float height = 900;
	InitWindow(width, height, "Ray Marching");

	if (!IsWindowReady())
	{
		LOG_ERR("Error while initializing the window");
		CloseWindow();
		return -1;
	}

	if (glewInit() != GLEW_OK)
	{
		LOG_ERR("Error while initializing GLEW");
		CloseWindow();
		return -1;
	}
	SetWindowState(FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(144);
	glViewport(0, 0, width, height);

	std::unique_ptr<Util::Shader> RayMarchingShader = std::make_unique<Util::Shader>("RayMarching.vs", "RayMarching.fs");
	
	GLuint vbo, vao;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	float vertices[] = { 
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glm::vec3 cameraPos(0.0f);
	float speed = 4.0f;
	float FpsTextScale = 18.0f;

	float timevar = 0;

	while (!WindowShouldClose())
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

		BeginDrawing();

		DrawText((std::to_string(GetFPS()) + " FPS").c_str(), GetScreenWidth() * 0.01f, GetScreenHeight() * 0.01f, FpsTextScale, WHITE);

		glUseProgram(RayMarchingShader->GetID());
		glBindVertexArray(vao);

		float dt = GetFrameTime();

		if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyDown(KEY_LEFT_SHIFT))
		{
			speed = 6.0f;
		}
		else if (IsKeyReleased(KEY_LEFT_SHIFT))
		{
			speed = 4.0f;
		}

		if (IsKeyPressed(KEY_W) || IsKeyDown(KEY_W))
		{
			cameraPos.z += speed * dt;
		}
		if (IsKeyPressed(KEY_S) || IsKeyDown(KEY_S))
		{
			cameraPos.z -= speed * dt;
		}
		if (IsKeyPressed(KEY_D) || IsKeyDown(KEY_D))
		{
			cameraPos.x += speed * dt;
		}
		if (IsKeyPressed(KEY_A) || IsKeyDown(KEY_A))
		{
			cameraPos.x -= speed * dt;
		}

		if (IsKeyPressed(KEY_SPACE) || IsKeyDown(KEY_SPACE))
		{
			cameraPos.y += speed * dt;
		}
		if (IsKeyPressed(KEY_LEFT_CONTROL) || IsKeyDown(KEY_LEFT_CONTROL))
		{
			cameraPos.y -= speed * dt;
		}
		
		glUniform2f(glGetUniformLocation(RayMarchingShader->GetID(), "WindowSize"), GetScreenWidth(), GetScreenHeight());
		glUniform2f(glGetUniformLocation(RayMarchingShader->GetID(), "MousePos"), GetMouseX(), GetMouseY());
		glUniform3f(glGetUniformLocation(RayMarchingShader->GetID(), "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
		glUniform1f(glGetUniformLocation(RayMarchingShader->GetID(), "time"), timevar);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindVertexArray(0);
		glUseProgram(0);
		EndDrawing();

		timevar += 1.0f * dt;

		if (timevar >= 360.0f)
		{
			timevar = 0.0f;
		}
	}

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(RayMarchingShader->GetID());

	LOG_INF("All resources are terminated!");
	CloseWindow();
	return 0;
}
