#pragma once
#include "framework.h"

struct GlobalLight {
	DirectX::XMFLOAT3 direction; // 12 bytes
	float pad1;                  // 4 bytes (padding to align next member)

	DirectX::XMFLOAT4 ambient;    // 16 bytes
	DirectX::XMFLOAT4 lightColor; // 16 bytes

	float intensity;              // 4 bytes
	float pad2[3];                // 12 bytes (padding to align struct to 16-byte boundary)
};