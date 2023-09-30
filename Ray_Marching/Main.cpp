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
	
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
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

	float MonitorWidth = GetMonitorWidth(GetCurrentMonitor());
	float MonitorHeight = GetMonitorHeight(GetCurrentMonitor());

	SetWindowState(FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(144);
	glViewport(0, 0, MonitorWidth, MonitorHeight);

	glEnable(GL_MULTISAMPLE);

	std::unique_ptr<Util::Shader> RayMarchingShader = std::make_unique<Util::Shader>("RayMarching.vs", "RayMarching.fs");
	RenderTexture2D ViewPortFBO = LoadRenderTexture(MonitorWidth, MonitorHeight);
	SetTextureFilter(ViewPortFBO.texture, TEXTURE_FILTER_BILINEAR);

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
	float scale = 0;
	float CameraSensitivity = 20.0f;

	bool CameraMode = true;
	bool AllowCaptureEnter = true;

	Vec2<float> CameraRadian({ 0,0 });

	LOG_INF("PRESS ENTER TO TURN OFF THE CAMERA MODE");

	while (!WindowShouldClose())
	{
		if (IsWindowMaximized())
		{
			scale = MIN((float)GetScreenWidth() / ViewPortFBO.depth.width, ((float)GetScreenHeight() + (ViewPortFBO.depth.height - GetScreenHeight())) / ViewPortFBO.depth.height);
		}
		if (IsWindowFullscreen())
		{
			scale = MIN((float)GetScreenWidth() / ViewPortFBO.depth.width, ((float)GetScreenHeight() + (ViewPortFBO.depth.height - GetScreenHeight())) / ViewPortFBO.depth.height);
		}
		else
		{
			scale = MIN((float)GetScreenWidth() / MonitorWidth, ((float)GetScreenHeight()) / MonitorHeight);
		}

		BeginTextureMode(ViewPortFBO);

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

		DrawText((std::to_string(GetFPS()) + " FPS").c_str(), MonitorWidth * 0.01f, MonitorHeight * 0.01f, FpsTextScale, WHITE);
		DrawText(("CAMERA MODE: " + std::to_string(CameraMode)).c_str(), MonitorWidth * 0.91f, MonitorHeight * 0.01f, FpsTextScale, WHITE);


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

		if ((IsKeyPressed(KEY_ENTER) || IsKeyDown(KEY_ENTER)) && AllowCaptureEnter)
		{
			CameraMode = !CameraMode;
			AllowCaptureEnter = false;
		}
		if (IsKeyReleased(KEY_ENTER))
		{
			AllowCaptureEnter = true;
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

		glUniform2f(glGetUniformLocation(RayMarchingShader->GetID(), "WindowSize"), MonitorWidth,MonitorHeight);
		glUniform2f(glGetUniformLocation(RayMarchingShader->GetID(), "MousePos"), GetMouseX(), GetMouseY());
		glUniform3f(glGetUniformLocation(RayMarchingShader->GetID(), "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
		glUniform1f(glGetUniformLocation(RayMarchingShader->GetID(), "time"), timevar);
		glUniform1f(glGetUniformLocation(RayMarchingShader->GetID(), "scaleCoeff"), (width / height) / (MonitorWidth / MonitorHeight));
		glUniform2f(glGetUniformLocation(RayMarchingShader->GetID(), "cameraRadian"), CameraRadian.x, CameraRadian.y);

		float aspect_ratio_hw = (float)MonitorWidth / MonitorHeight;
		float aspect_ratio_wh = (float)MonitorHeight / MonitorWidth;

		glm::mat4 ImageScaleRatioMat(1.0f);
		ImageScaleRatioMat = glm::scale(ImageScaleRatioMat, glm::vec3(aspect_ratio_wh, 1.0f, 1.0f));

		glUniformMatrix4fv(glGetUniformLocation(RayMarchingShader->GetID(), "modelMat"), 1, GL_FALSE, glm::value_ptr(ImageScaleRatioMat));

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindVertexArray(0);
		glUseProgram(0);

		EndTextureMode();

		BeginDrawing();

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

		if (IsWindowMaximized())
		{
			DrawTexturePro(ViewPortFBO.texture, { 0.0f, 0.0f, (float)ViewPortFBO.texture.width, (float)-ViewPortFBO.texture.height }, { (GetScreenWidth() - ((float)GetMonitorWidth(GetCurrentMonitor()) * scale)) * 0.5f - (ViewPortFBO.depth.height - GetScreenHeight()), (GetScreenHeight() - ((float)GetMonitorHeight(GetCurrentMonitor()) * scale)) * 0.5f,(float)GetMonitorWidth(GetCurrentMonitor()) * scale + (2 * (ViewPortFBO.depth.height - GetScreenHeight())), (float)GetMonitorHeight(GetCurrentMonitor()) * scale }, { 0, 0 }, 0.0f, WHITE);
		}
		else if (IsWindowFullscreen())
		{
			DrawTexturePro(ViewPortFBO.texture, { 0.0f, 0.0f, (float)ViewPortFBO.texture.width, (float)-ViewPortFBO.texture.height }, { (GetScreenWidth() - ((float)GetMonitorWidth(GetCurrentMonitor()) * scale)) * 0.5f, (GetScreenHeight() - ((float)GetMonitorHeight(GetCurrentMonitor()) * scale)) * 0.5f,(float)GetMonitorWidth(GetCurrentMonitor()) * scale, (float)GetMonitorHeight(GetCurrentMonitor()) * scale + (ViewPortFBO.depth.height - GetScreenHeight()) }, { 0, 0 }, 0.0f, WHITE);
		}
		else if (!IsWindowMaximized() && !IsWindowFullscreen())
		{
			DrawTexturePro(ViewPortFBO.texture, { 0.0f, 0.0f, (float)ViewPortFBO.texture.width, (float)-ViewPortFBO.texture.height }, { (GetScreenWidth() - ((float)GetMonitorWidth(GetCurrentMonitor()) * scale)) * 0.5f, (GetScreenHeight() - ((float)GetMonitorHeight(GetCurrentMonitor()) * scale)) * 0.5f,(float)GetMonitorWidth(GetCurrentMonitor()) * scale, (float)GetMonitorHeight(GetCurrentMonitor()) * scale }, { 0, 0 }, 0.0f, WHITE);
		}
		
		EndDrawing();

		timevar += 1.0f * dt;

		if (timevar >= 360.0f)
		{
			timevar = 0.0f;
		}

		if (CameraMode)
		{
			HideCursor();
			Vec2<float> mouseDelta({ GetMouseDelta().x * dt ,GetMouseDelta().y * dt });
			CameraRadian(CameraRadian + (mouseDelta * CameraSensitivity));

			if (abs(glm::radians(CameraRadian.x)) >= 6.28318531)
			{
				CameraRadian.x = 0;
			}
			if (abs(glm::radians(CameraRadian.y)) >= 6.28318531)
			{
				CameraRadian.y = 0;
			}

			SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);

		}
		else
		{
			ShowCursor();
		}
	}

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(RayMarchingShader->GetID());
	UnloadRenderTexture(ViewPortFBO);

	LOG_INF("All resources are terminated!");
	CloseWindow();
	return 0;
}
