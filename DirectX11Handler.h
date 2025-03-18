#pragma once
#include "framework.h"

#include "DirectX11Types.h"

#include "Window.h"

struct FrameData {
    DirectX::XMMATRIX model;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
};

struct DirectX11HandlerDescription {
    UINT backBufferCount = 2;
    BOOL vSync           = FALSE;
};

class DirectX11Handler {
private:
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;

    Microsoft::WRL::ComPtr<ID3D11Device>        device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;

    D3D11_INPUT_ELEMENT_DESC layout[3] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    D3D11_VIEWPORT viewport;
    BOOL           vSync;

    Window* window;

public:
    DirectX::XMMATRIX modelMatrix;
    DirectX::XMMATRIX viewMatrix;
    DirectX::XMMATRIX projectionMatrix;

    DirectX11Handler(Window* window, const DirectX11HandlerDescription& description) :
        window(window), vSync(description.vSync)
    {
        HRESULT hr;

        UINT createDeviceFlags = 0;
#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        DXGI_SWAP_CHAIN_DESC swapChainDescription = {};
        swapChainDescription.BufferCount          = description.backBufferCount;
        swapChainDescription.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDescription.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDescription.SampleDesc.Count     = 1;
        swapChainDescription.BufferDesc.Width     = window->width;
        swapChainDescription.BufferDesc.Height    = window->height;
        swapChainDescription.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDescription.OutputWindow         = window->hWnd;
        swapChainDescription.Windowed             = TRUE;

        hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                           D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                           createDeviceFlags, nullptr, 0, D3D11_SDK_VERSION,
                                           &swapChainDescription, this->swapChain.GetAddressOf(), this->device.GetAddressOf(), nullptr, this->context.GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create depth stencil view");

        this->createRenderTargetView();
        this->createDepthStencil(DXGI_FORMAT_D24_UNORM_S8_UINT);

        this->setViewport(window->width, window->height);

#ifdef _DEBUG
        std::string message = std::format(
            R"(DirectX 11 Handler successfully created!
- Device:             {}
- Context:            {}
- Swap Chain:         {}
- Depth Stencil View: {}
- Render Target View: {}
- Viewport:           {})",
            static_cast<void*>(this->device.Get()),
            static_cast<void*>(this->context.Get()),
            static_cast<void*>(this->swapChain.Get()),
            static_cast<void*>(this->depthStencilView.Get()),
            static_cast<void*>(this->renderTargetView.Get()),
            static_cast<void*>(&this->viewport)
        );
        RC_DBG_LOG(message);
#endif
    }

    void createRenderTargetView() {
        HRESULT hr;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        hr = this->swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        RC_EI_ASSERT(FAILED(hr), "Failed to create depth stencil view");

        hr = this->device->CreateRenderTargetView(backBuffer.Get(), nullptr, this->renderTargetView.GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create render target view");
    }

    void bindShaderResource(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) {
        this->context->PSSetShaderResources(0, 1, srv.GetAddressOf());
    }
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> createDepthStencilBuffer(const DXGI_FORMAT format) {
        HRESULT hr;

        DXGI_SWAP_CHAIN_DESC swapChainDescription = {};
        this->swapChain->GetDesc(&swapChainDescription);

        D3D11_TEXTURE2D_DESC depthStencilDescription = {};
        depthStencilDescription.Width                = swapChainDescription.BufferDesc.Width;
        depthStencilDescription.Height               = swapChainDescription.BufferDesc.Height;
        depthStencilDescription.MipLevels            = 1;
        depthStencilDescription.ArraySize            = 1;
        depthStencilDescription.Format               = format;
        depthStencilDescription.SampleDesc           = swapChainDescription.SampleDesc;
        depthStencilDescription.BindFlags            = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
        hr = this->device->CreateTexture2D(&depthStencilDescription, nullptr, depthStencilBuffer.GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create depth stencil buffer");

        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
        hr = this->device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, depthStencilView.GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create depth stencil view");

        return depthStencilView;
    }
    void createDepthStencil(const DXGI_FORMAT format) {
        HRESULT hr;

        DXGI_SWAP_CHAIN_DESC swapChainDescription = {};
        this->swapChain->GetDesc(&swapChainDescription);

        D3D11_TEXTURE2D_DESC depthStencilDescription = {};
        depthStencilDescription.Width                = swapChainDescription.BufferDesc.Width;
        depthStencilDescription.Height               = swapChainDescription.BufferDesc.Height;
        depthStencilDescription.MipLevels            = 1;
        depthStencilDescription.ArraySize            = 1;
        depthStencilDescription.Format               = format;
        depthStencilDescription.SampleDesc           = swapChainDescription.SampleDesc;
        depthStencilDescription.BindFlags            = D3D11_BIND_DEPTH_STENCIL;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
        hr = this->device->CreateTexture2D(&depthStencilDescription, nullptr, depthStencilBuffer.GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create depth stencil buffer");

        hr = this->device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, this->depthStencilView.GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create depth stencil view");

        D3D11_DEPTH_STENCIL_DESC depthStencilStateDescription = {};

        depthStencilStateDescription.DepthEnable    = TRUE;
        depthStencilStateDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilStateDescription.DepthFunc      = D3D11_COMPARISON_LESS;

        depthStencilStateDescription.StencilEnable    = TRUE;
        depthStencilStateDescription.StencilReadMask  = 0xFF;
        depthStencilStateDescription.StencilWriteMask = 0xFF;

        depthStencilStateDescription.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
        depthStencilStateDescription.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        depthStencilStateDescription.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        depthStencilStateDescription.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;

        depthStencilStateDescription.BackFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
        depthStencilStateDescription.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        depthStencilStateDescription.BackFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        depthStencilStateDescription.BackFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;

        hr = this->device->CreateDepthStencilState(&depthStencilStateDescription, this->depthStencilState.GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create depth stencil state");
    }

    void prepareToRenderShadowMaps(Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencil) {
        this->context->OMSetRenderTargets(0, nullptr, depthStencil.Get());
        this->context->OMSetDepthStencilState(this->depthStencilState.Get(), 0);
    }
    void prepare() {
        this->context->OMSetRenderTargets(1, this->renderTargetView.GetAddressOf(), this->depthStencilView.Get());
        this->context->OMSetDepthStencilState(this->depthStencilState.Get(), 0);
    }

    Shader createShadersFromSource(const std::string& vertexShaderSource, const std::string& pixelShaderSource) {
        HRESULT hr;

        Shader shader = {};

        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob;

        hr = D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.size() + 1,
                        nullptr, nullptr, nullptr,
                        "VSMain", "vs_5_0", 0, 0,
                        &vertexShaderBlob, pErrorBlob.GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to compile vertex shader\n" << (const char*)pErrorBlob->GetBufferPointer());

        hr = D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.size() + 1,
                        nullptr, nullptr, nullptr,
                        "PSMain", "ps_5_0", 0, 0,
                        &pixelShaderBlob, pErrorBlob.GetAddressOf());
        RC_EI_ASSERT(FAILED(hr) && pErrorBlob, "Failed to compile pixel shader. Error: " << static_cast<const char*>(pErrorBlob->GetBufferPointer()));

        hr = this->device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(),
                                              nullptr, &shader.vertexShader);
        RC_EI_ASSERT(FAILED(hr), "Failed to create vertex shader");
        hr = this->device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(),
                                             nullptr, &shader.pixelShader);
        RC_EI_ASSERT(FAILED(hr), "Failed to create pixel shader");

        createInputLayout(&shader.inputLayout, vertexShaderBlob, pixelShaderBlob);

        return shader;
    }

    Mesh createVertexArrayBuffer(std::vector<Vertex>& vertices, std::vector<DWORD>& indices) {
        HRESULT hr;

        Mesh mesh     = {};
        mesh.vertices = vertices;
        mesh.indices  = indices;

        D3D11_BUFFER_DESC vertexArrayBufferDescription = {};
        vertexArrayBufferDescription.Usage             = D3D11_USAGE_DEFAULT;
        vertexArrayBufferDescription.ByteWidth         = vertices.size() * sizeof(Vertex);
        vertexArrayBufferDescription.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
        vertexArrayBufferDescription.CPUAccessFlags    = NULL;

        D3D11_BUFFER_DESC indexArrayBufferDescription = {};
        indexArrayBufferDescription.Usage             = D3D11_USAGE_DEFAULT;
        indexArrayBufferDescription.ByteWidth         = indices.size() * sizeof(DWORD);
        indexArrayBufferDescription.BindFlags         = D3D11_BIND_INDEX_BUFFER;
        indexArrayBufferDescription.CPUAccessFlags    = NULL;

        mesh.vertexInitData.pSysMem = mesh.vertices.data();
        hr = this->device->CreateBuffer(&vertexArrayBufferDescription, &mesh.vertexInitData, &mesh.vertexArrayBuffer);
        RC_EI_ASSERT(FAILED(hr), "Failed to create vertex array buffer");

        mesh.indexInitData.pSysMem = mesh.indices.data();
        hr = this->device->CreateBuffer(&indexArrayBufferDescription, &mesh.indexInitData, &mesh.indexArrayBuffer);
        RC_EI_ASSERT(FAILED(hr), "Failed to create index array buffer");

        return mesh;
    }

    void createInputLayout(Microsoft::WRL::ComPtr<ID3D11InputLayout>* inputLayout, 
                           Microsoft::WRL::ComPtr<ID3DBlob>          vertexShaderBlob, 
                           Microsoft::WRL::ComPtr<ID3DBlob>          pixelShaderBlob)
    {
        HRESULT hr;
        hr = this->device->CreateInputLayout(this->layout, ARRAYSIZE(this->layout),
                                             vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(),
                                             inputLayout->GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create input layout");
    }

    void clearDepthStencil() {
        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;

        Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
        device->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf());
        context->RSSetState(rasterizerState.Get());

        this->context->ClearDepthStencilView(this->depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }
    void clearScreen() {
        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;

        Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState = nullptr;
        device->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf());
        context->RSSetState(rasterizerState.Get());

        this->context->ClearRenderTargetView(this->renderTargetView.Get(), DirectX::Colors::CadetBlue);
        this->context->ClearDepthStencilView(this->depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    void createTexture2D(Microsoft::WRL::ComPtr<ID3D11Texture2D>*     outTexture,
        std::unique_ptr<unsigned char[], decltype(&stbi_image_free)>* outImageData,
                         const char*                                  path, 
                         const DXGI_FORMAT                            format)
    {
        HRESULT hr;

        int width;
        int height;
        int channels;

        std::unique_ptr<unsigned char[], decltype(&stbi_image_free)> imageData(
            stbi_load(path, &width, &height, &channels, STBI_rgb_alpha),
            stbi_image_free);
        RC_EI_ASSERT(!imageData, "Failed to load image data.");

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width                = width;
        textureDesc.Height               = height;
        textureDesc.MipLevels            = 1;
        textureDesc.ArraySize            = 1;
        textureDesc.Format               = format;
        textureDesc.SampleDesc.Count     = 1;
        textureDesc.SampleDesc.Quality   = 0;
        textureDesc.Usage                = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags       = NULL;
        textureDesc.MiscFlags            = NULL;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem                = imageData.get();
        initData.SysMemPitch            = width * 4;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        hr = this->device->CreateTexture2D(&textureDesc, &initData, texture.GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create texture 2D.");
        
        *outTexture   = texture;
        *outImageData = std::move(imageData);
    }
    ID3D11Texture2D* getTextureFromShaderResourceView(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView) {
        HRESULT hr;

        ID3D11Resource* resource;
        shaderResourceView->GetResource(&resource);

        ID3D11Texture2D* texture;

        hr = resource->QueryInterface(_uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture));
        RC_EI_ASSERT(FAILED(hr), "Failed to get texture 2D from shader resource view.");

        return texture;
    }

    void createShaderResourceView(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* outShaderResourceView,
                                  Microsoft::WRL::ComPtr<ID3D11Resource>            resource) 
    {
        HRESULT hr;

        hr = this->device->CreateShaderResourceView(resource.Get(), nullptr, outShaderResourceView->GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create shader resource view.");
    }

    void createSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>* outSamplerState) {
        HRESULT hr;

        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // Linear filtering for min/mag/mip
        samplerDesc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;  // Wrap texture coordinates horizontally
        samplerDesc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;  // Wrap texture coordinates vertically
        samplerDesc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.ComparisonFunc     = D3D11_COMPARISON_NEVER;
        samplerDesc.MinLOD             = 0;
        samplerDesc.MaxLOD             = D3D11_FLOAT32_MAX;

        hr = this->device->CreateSamplerState(&samplerDesc, outSamplerState->GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create sampler state.");
    }
    void bindSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState) {
        RC_WI_ASSERT(!samplerState.Get(), "Sampler state is null. Did you mean to bind a sampler state?");
        this->context->PSSetSamplers(0, 1, samplerState.GetAddressOf());
    }

    void render(Shader& shader, Mesh& mesh) {
        RC_WI_ASSERT(!shader.vertexShader.Get(), "The vertex shader passed to the render function is not valid.");
        RC_WI_ASSERT(!shader.pixelShader.Get(), "The pixel shader passed to the render function is not valid.");
        RC_WI_ASSERT(!shader.inputLayout.Get(), "The input layout passed to the render function is not valid.");

        RC_WI_ASSERT(!mesh.vertexArrayBuffer.Get(), "The vertex array buffer passed to the render function is not valid.");
        RC_WI_ASSERT(!mesh.indexArrayBuffer.Get(), "The index array buffer passed to the render function is not valid.");
        RC_WI_ASSERT(mesh.vertices.empty(), "The vertex data size is 0.");
        RC_WI_ASSERT(mesh.vertices.empty(), "The index data size is 0.");

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        this->context->VSSetShader(shader.vertexShader.Get(), nullptr, 0);
        this->context->PSSetShader(shader.pixelShader.Get(), nullptr, 0);

        this->context->IASetVertexBuffers(0, 1, mesh.vertexArrayBuffer.GetAddressOf(), &stride, &offset);
        this->context->IASetIndexBuffer(mesh.indexArrayBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        this->context->IASetInputLayout(shader.inputLayout.Get());
        this->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        this->context->DrawIndexed(mesh.indices.size(), 0, 0);
    }

    void present() {
        this->swapChain->Present(vSync, 0);
    }

    void setViewport(const UINT width, const UINT height) {
        viewport.Width    = width;
        viewport.Height   = height;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        this->context->RSSetViewports(1, &this->viewport);
    }
    
    template <typename StructType>
    Buffer createConstantBuffer(StructType* data) {
        RC_WI_ASSERT(!data, "The data passed to the create constant buffer is not valid!");

        HRESULT hr;

        Buffer cbuffer = {};
        D3D11_BUFFER_DESC cbufferDesc   = {};
        cbufferDesc.Usage               = D3D11_USAGE_DYNAMIC;          // Default usage (GPU read-only)
        cbufferDesc.ByteWidth           = sizeof(StructType);           // Size of the constant buffer
        cbufferDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;   // Bind as constant buffer
        cbufferDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;  
        cbufferDesc.MiscFlags           = 0;                            // No miscellaneous flags
        cbufferDesc.StructureByteStride = 0;                            // Not applicable for constant buffers

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.SysMemPitch            = 0;
        initData.SysMemSlicePitch       = 0;
        initData.pSysMem                = data;

        hr = this->device->CreateBuffer(&cbufferDesc, &initData, &cbuffer.buffer);
        RC_EI_ASSERT(FAILED(hr), "Failed to create constant buffer.");
        
        return cbuffer;
    }

    void VSBindBuffers(std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>>& buffers) {
        if (buffers.empty()) return;
        context->VSSetConstantBuffers(0, buffers.size(), buffers.data()->GetAddressOf()); // Vertex Shader
    }
    void PSBindBuffers(std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>>& buffers) {
        if (buffers.empty()) return;
        context->PSSetConstantBuffers(0, buffers.size(), buffers.data()->GetAddressOf());
    }
    
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> getDeviceContext() { return this->context.Get(); }
    Microsoft::WRL::ComPtr<IDXGISwapChain> getSwapChain() { return this->swapChain.Get(); }
    Microsoft::WRL::ComPtr<ID3D11Device> getDevice() { return this->device.Get(); }
};