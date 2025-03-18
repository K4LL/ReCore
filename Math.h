#pragma once
#include "framework.h"

#include <DirectXCollision.h>

#include <intrin.h>

using Quaternion = __m128;

struct Float3 {
public:
	float x, y, z;

	Float3()						= default;
	Float3(const Float3& other)     = default;
	Float3(Float3&& other) noexcept = default;

	Float3& operator=(const Float3& other)     = default;
	Float3& operator=(Float3&& other) noexcept = default;

	Float3(const float x, const float y, const float z) :
		x(x), y(y), z(z) {}
};
struct Float3x3 {
public:
	union {
		struct {
			float x1, y1, z1;
			float x2, y2, z2;
			float x3, y3, z3;
		};
		float mat[3][3];
	};

	Float3x3()        				    = default;
	Float3x3(const Float3x3& other)     = default;
	Float3x3(Float3x3&& other) noexcept = default;

	Float3x3& operator=(const Float3x3& other)     = default;
	Float3x3& operator=(Float3x3&& other) noexcept = default;

	Float3x3(const float x1, const float y1, const float z1,
			 const float x2, const float y2, const float z2,
			 const float x3, const float y3, const float z3) :
		x1(x1), y1(y1), z1(z1), 
		x2(x2), y2(y2), z2(z2), 
		x3(x3), y3(y3), z3(z3) {}

	float operator()(const std::size_t row, const std::size_t collumn) const noexcept { return mat[row][collumn]; }
	float& operator()(const std::size_t row, const std::size_t collumn) noexcept { return mat[row][collumn]; }

	operator DirectX::XMFLOAT3X3() { return static_cast<DirectX::XMFLOAT3X3>(*this); }
};

class Math {
public:
	static bool isCubeColliding(Transform t1, Transform t2)
	{
		DirectX::BoundingOrientedBox box1;
		DirectX::BoundingOrientedBox box2;

		DirectX::XMFLOAT3 t1Scale;
		DirectX::XMFLOAT3 t1Position;
		DirectX::XMFLOAT4 t1Rotation;
		DirectX::XMFLOAT3 t2Scale;
		DirectX::XMFLOAT3 t2Position;
		DirectX::XMFLOAT4 t2Rotation;

		DirectX::XMStoreFloat3(&t1Scale, t1.scale);
		DirectX::XMStoreFloat3(&t1Position, t1.position);
		DirectX::XMStoreFloat4(&t1Rotation, t1.rotation);
		DirectX::XMStoreFloat3(&t2Scale, t2.scale);
		DirectX::XMStoreFloat3(&t2Position, t2.position);
		DirectX::XMStoreFloat4(&t2Rotation, t2.rotation);

		// Set up box1 using t1 data.
		box1.Center = t1Position;
		box1.Extents = DirectX::XMFLOAT3(t1Scale.x * 0.5f, t1Scale.y * 0.5f, t1Scale.z * 0.5f);
		box1.Orientation = t1Rotation;

		// Set up box2 using t2 data.
		box2.Center = t2Position;
		box2.Extents = DirectX::XMFLOAT3(t2Scale.x * 0.5f, t2Scale.y * 0.5f, t2Scale.z * 0.5f);  // corrected here
		box2.Orientation = t2Rotation;

		return box1.Intersects(box2);
	}

	static void __vectorcall QuaternionToEulerAngles(const DirectX::XMVECTOR& q, float* roll, float* pitch, float* yaw) {
		float x = DirectX::XMVectorGetX(q);
		float y = DirectX::XMVectorGetY(q);
		float z = DirectX::XMVectorGetZ(q);
		float w = DirectX::XMVectorGetW(q);

		// Roll (X-axis rotation)
		float sinr_cosp = 2.0f * (w * x + y * z);
		float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
		*roll = atan2f(sinr_cosp, cosr_cosp);

		// Pitch (Y-axis rotation)
		float sinp = 2.0f * (w * y - z * x);
		if (fabsf(sinp) >= 1)
			*pitch = copysignf(DirectX::XM_PI / 2, sinp);  // Use 90 degrees if out of range
		else
			*pitch = asinf(sinp);

		// Yaw (Z-axis rotation)
		float siny_cosp = 2.0f * (w * z + x * y);
		float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
		*yaw = atan2f(siny_cosp, cosy_cosp);
	}

	static float radToDeg(const float rads) {
		return rads * (180.0f / DirectX::XM_PI);
	}
	static float degToRad(const float degs) {
		return degs * (DirectX::XM_PI / 180.0f);
	}

	static DirectX::XMVECTOR __vectorcall rotate(Quaternion quat1, DirectX::XMFLOAT3 rot) {
		DirectX::XMVECTOR quat2 = DirectX::XMQuaternionIdentity();
		quat2                   = DirectX::XMQuaternionRotationRollPitchYaw(rot.x, rot.y, rot.z);

		quat1 = DirectX::XMQuaternionMultiply(quat1, quat2);
		quat1 = DirectX::XMQuaternionNormalize(quat1);

		return quat1;
	}
	static DirectX::XMVECTOR __vectorcall rotate(Quaternion quat1, Quaternion quat2) {
		quat1 = DirectX::XMQuaternionMultiply(quat1, quat2);
		quat1 = DirectX::XMQuaternionNormalize(quat1);

		return quat1;
	}
};