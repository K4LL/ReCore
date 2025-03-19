#pragma once
#include <intrin.h>

#include <windows.h>

#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>

#include <wrl/client.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "stb_image.h"

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

#include <thread>
#include <functional>
#include <variant>

#include "RCTime.h"

#ifdef _DEBUG
#define RC_I_FL_HELPER "(" << __FILE__ << ":" << __LINE__ << ")" 

#define RC_DBG_LOG(x) std::cout << "[INFO] " << RC_I_FL_HELPER << " " << x << '\n'
#define RC_DBG_WARN(x) std::cout << "[WARN] " << RC_I_FL_HELPER << " " << x << '\n'
#define RC_DBG_ERROR(x) std::cout << "[ERROR] " << RC_I_FL_HELPER << " " << x << '\n'

#define RC_L_ASSERT(x, y) if (x) std::cout << "[INFO] " << y << '\n'
#define RC_W_ASSERT(x, y) if (x) std::cout << "[WARN] " << y << '\n'
#define RC_E_ASSERT(x, y) if (x) std::cout << "[ERROR] " << y << '\n'

#define RC_LI_ASSERT(x, y) if (x) std::cout << "[INFO] " << RC_I_FL_HELPER << " " << y << '\n'
#define RC_WI_ASSERT(x, y) if (x) std::cout << "[WARN] " << RC_I_FL_HELPER << " " << y << '\n'
#define RC_EI_ASSERT(x, y) if (x) std::cout << "[ERROR] " << RC_I_FL_HELPER << " " << y << '\n'

#define RC_LR_ASSERT(x, y, z) if (x) { std::cout << "[INFO] " << y << '\n'; return z; }
#define RC_WR_ASSERT(x, y, z) if (x) { std::cout << "[WARN] " << y << '\n'; return z; }
#define RC_ER_ASSERT(x, y, z) if (x) { std::cout << "[ERROR] " << y << '\n'; return z; }

#define RC_LIR_ASSERT(x, y, z) if (x) { std::cout << "[INFO] " << RC_I_FL_HELPER << " " << y << '\n'; return z; }
#define RC_WIR_ASSERT(x, y, z) if (x) { std::cout << "[WARN] " << RC_I_FL_HELPER << " " << y << '\n'; return z; }
#define RC_EIR_ASSERT(x, y, z) if (x) { std::cout << "[ERROR] " << RC_I_FL_HELPER << " " << y << '\n'; return z; }

#define RC_DBG_VAR(x) x
#define RC_DBG_CODE(x) x

#else
#define RC_L_ASSERT(x, y)
#define RC_W_ASSERT(x, y)
#define RC_E_ASSERT(x, y)

#define RC_LI_ASSERT(x, y)
#define RC_WI_ASSERT(x, y)
#define RC_EI_ASSERT(x, y)

#define RC_DBG_LOG(x)
#define RC_DBG_WARN(x)
#define RC_DBG_ERROR(x)

#define RC_LR_ASSERT(x, y)
#define RC_WR_ASSERT(x, y)
#define RC_ER_ASSERT(x, y)

#define RC_LIR_ASSERT(x, y)
#define RC_WIR_ASSERT(x, y)
#define RC_EIR_ASSERT(x, y)

#define RC_DBG_VAR(x)
#define RC_DBG_CODE(x)
#endif

struct alignas(16) EulerAngles {
	float pitch;
	float yaw;
	float roll;
};