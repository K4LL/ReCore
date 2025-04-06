#pragma once
#include "framework.h"

#include "Window.h"
#include "DirectX11Types.h"

#include <algorithm>

class Camera {
public:
	Window* window;

	float nearPlane;
    float farPlane;

	DirectX::XMMATRIX projectionMatrix;
	DirectX::XMMATRIX viewMatrix;

	Transform   transform;
	EulerAngles eulerAngles;

	DirectX::XMVECTOR upVector;

	bool  isMouseRotating;
	POINT lastMousePos;

	Camera(Window* window) :
		window(window),
		nearPlane(0.1f), farPlane(1000.0f),
		projectionMatrix(DirectX::XMMatrixIdentity()), viewMatrix(DirectX::XMMatrixIdentity()),
		upVector(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f))
	{
		transform.position = DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);

		this->viewMatrix = DirectX::XMMatrixLookAtLH(
			this->transform.position,
			DirectX::XMVectorAdd(this->transform.position, DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), this->transform.rotation)),
			this->upVector
		);

		this->projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(45.0f),
			static_cast<float>(this->window->width) / static_cast<float>(this->window->height), // Aspect ratio
			this->nearPlane,
			this->farPlane
		);
	}
	Camera(const int		 nearPlane,
		   const int		 farPlane,
		   DirectX::XMMATRIX projectionMatrix,
		   DirectX::XMMATRIX viewMatrix,
		   Transform		 transform,
		   DirectX::XMVECTOR upVector) :
		nearPlane(nearPlane), farPlane(farPlane),
		projectionMatrix(projectionMatrix), viewMatrix(viewMatrix),
		transform(transform),
		upVector(upVector)
	{
		this->viewMatrix = DirectX::XMMatrixLookAtLH(
			this->transform.position,
			DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
			this->upVector
		);

		this->projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(45.0f),
			static_cast<float>(this->window->width) / static_cast<float>(this->window->height), // Aspect ratio
			this->nearPlane,
			this->farPlane
		);
	}

	void update() {
		float lookSpeed = 0.5f;

		if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
			if (!isMouseRotating) {
				GetCursorPos(&lastMousePos);
				isMouseRotating = true;
			}
			else {
				// When processing mouse input:
				POINT currentMousePos;
				GetCursorPos(&currentMousePos);

				float deltaX = ((currentMousePos.x - lastMousePos.x) * lookSpeed) * RCTime::deltaTime();
				float deltaY = ((currentMousePos.y - lastMousePos.y) * lookSpeed) * RCTime::deltaTime();
				lastMousePos = currentMousePos;

				this->eulerAngles.yaw   += deltaX;
				this->eulerAngles.pitch += deltaY;

				const float maxPitch = DirectX::XMConvertToRadians(89.9f);
				this->eulerAngles.pitch = std::clamp(this->eulerAngles.pitch, -maxPitch, maxPitch);

				this->transform.rotation = DirectX::XMQuaternionRotationRollPitchYaw(this->eulerAngles.pitch, this->eulerAngles.yaw, 0.0f);
				this->transform.rotation = DirectX::XMQuaternionNormalize(this->transform.rotation);

				RC_DBG_LOG(this->eulerAngles.yaw);
				RC_DBG_LOG(this->eulerAngles.pitch);
			}
		}
		else {
			isMouseRotating = false;
		}

		static float speed;
		if (speed == 0.0f) {
			speed = 5.0f;
		}

		DirectX::XMFLOAT4 co;
		DirectX::XMStoreFloat4(&co, this->transform.rotation);
		
		DirectX::XMVECTOR forward = DirectX::XMVector3Rotate(
			DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), this->transform.rotation
		);
		DirectX::XMVECTOR right = DirectX::XMVector3Rotate(
			DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), this->transform.rotation
		);

		if (GetAsyncKeyState('W') & 0x8000) {
			this->transform.position = DirectX::XMVectorAdd(
				this->transform.position,
				DirectX::XMVectorScale(forward, speed * RCTime::deltaTime())
			);
		}		
		if (GetAsyncKeyState('S') & 0x8000) {
			this->transform.position = DirectX::XMVectorSubtract(
				this->transform.position,
				DirectX::XMVectorScale(forward, speed * RCTime::deltaTime())
			);
		}		
		if (GetAsyncKeyState('A') & 0x8000) {
			this->transform.position = DirectX::XMVectorAdd(
				this->transform.position,
				DirectX::XMVectorScale(right, speed * RCTime::deltaTime())
			);
		}		
		if (GetAsyncKeyState('D') & 0x8000) {
			this->transform.position = DirectX::XMVectorSubtract(
				this->transform.position,
				DirectX::XMVectorScale(right, speed * RCTime::deltaTime())
			);
		}		
		if (GetAsyncKeyState('E') & 0x8000) {
			this->transform.position = DirectX::XMVectorAdd(
				this->transform.position,
				DirectX::XMVectorScale(this->upVector, speed * RCTime::deltaTime())
			);
		}		
		if (GetAsyncKeyState('Q') & 0x8000) {
			this->transform.position = DirectX::XMVectorSubtract(
				this->transform.position,
				DirectX::XMVectorScale(this->upVector, speed * RCTime::deltaTime())
			);
		}	
		if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) {
			speed = 50.0f;
		}
		else {
			speed = 5.0f;
		}

		DirectX::XMVECTOR rotatedForward = DirectX::XMVector3Rotate(
			DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), this->transform.rotation
		);

		DirectX::XMVECTOR target = DirectX::XMVectorAdd(this->transform.position, rotatedForward);

		viewMatrix = DirectX::XMMatrixLookAtLH(transform.position, target, upVector);
		projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(45.0f),
			static_cast<float>(window->width) / static_cast<float>(window->height),
			nearPlane, farPlane
		);
	}
};