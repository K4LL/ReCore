#pragma once
#include "framework.h"

#include <typeindex>

struct Transform {
    DirectX::XMVECTOR position;
    DirectX::XMVECTOR rotation;
    DirectX::XMVECTOR scale;

    DirectX::XMMATRIX model;

    Transform() { 
        this->position = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f); // Default position
        this->scale    = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f); // Default scale
        this->rotation = DirectX::XMQuaternionIdentity();              // No rotation
        this->model    = DirectX::XMMatrixIdentity();                  // Identity matrix
    }
};

struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 uv;
};

struct Shader {
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  inputLayout;
};

struct Mesh {
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexArrayBuffer;
    D3D11_SUBRESOURCE_DATA vertexInitData;

    Microsoft::WRL::ComPtr<ID3D11Buffer> indexArrayBuffer;
    D3D11_SUBRESOURCE_DATA indexInitData;

    std::vector<Vertex> vertices;
    std::vector<DWORD>  indices;

    std::string path;
};

struct Texture {
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>             texture;
    std::unique_ptr<unsigned char[], decltype(&stbi_image_free)> image;

    std::string path;

    Texture() : texture(nullptr), image(nullptr, stbi_image_free) {}
};

struct Buffer {
    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    std::type_index                      type = typeid(void);
};

struct Model {
    Transform transform;
    Mesh      mesh;
    Shader    shader;
    Texture   texture;

    std::vector<Buffer> buffers;
};