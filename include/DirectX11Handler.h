#pragma once
#include "framework.h"

#include "DirectX11Types.h"

#include "Window.h"

#include <format>

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
    }

    void createRenderTargetView() {
        HRESULT hr;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        hr = this->swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        RC_EI_ASSERT(FAILED(hr), "Failed to create depth stencil view");

        hr = this->device->CreateRenderTargetView(backBuffer.Get(), nullptr, this->renderTargetView.GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create render target view");
    }

    void bindShaderResource(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> inShaderResourceView) {
        this->context->PSSetShaderResources(0, 1, inShaderResourceView.GetAddressOf());
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

        this->createInputLayout(&shader.inputLayout, vertexShaderBlob, pixelShaderBlob);

        return shader;
    }

    void createVertexArrayBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>* outVertexArrayBuffer, 
                                 std::vector<Vertex>*                  outVertices, 
                                 D3D11_SUBRESOURCE_DATA*               outInitData, 
                                 const std::vector<Vertex>&            inVertices) 
    {
        // Set asserts for debug builds
		RC_DBG_CODE(
            if (!outVertexArrayBuffer) {
                RC_DBG_ERROR("The OUT VERTEX ARRAY BUFFER passed to the create vertex array buffer function is 0.");
                return;
            }
            if (!outVertices) {
                RC_DBG_ERROR("The OUT VERTEX DATA passed to the create vertex array buffer function is 0.");
                return;
            }
            if (!outInitData) {
                RC_DBG_ERROR("The OUT SUBRESOURCE DATA passed to the create vertex array buffer function is 0.");
                return;
            }
            if (inVertices.empty()) {
                RC_DBG_ERROR("The IN VERTEX DATA passed to the create vertex array buffer function is empty.");
                return;
            }
        );

        HRESULT hr;

		// Be sure this happens before the vertex array buffer is created
        *outVertices = inVertices;

        D3D11_BUFFER_DESC vertexArrayBufferDescription = {};
        vertexArrayBufferDescription.Usage             = D3D11_USAGE_DEFAULT;
        vertexArrayBufferDescription.ByteWidth         = outVertices->size() * sizeof(Vertex);
        vertexArrayBufferDescription.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
        vertexArrayBufferDescription.CPUAccessFlags    = NULL;

		// Set the data for the vertex array buffer
        outInitData->pSysMem = outVertices->data();
        hr = this->device->CreateBuffer(&vertexArrayBufferDescription, outInitData, outVertexArrayBuffer->GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create vertex array buffer");
    }
    void createIndexArrayBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>* outIndexArrayBuffer, 
                                std::vector<uint32_t>*                outIndices, 
                                D3D11_SUBRESOURCE_DATA*               outInitData, 
                                const std::vector<uint32_t>&          inIndices) 
    {
        // Set asserts for debug builds
        RC_DBG_CODE(
            if (!outIndexArrayBuffer) {
                RC_DBG_ERROR("The OUT INDEX ARRAY BUFFER passed to the create index array buffer function is 0.");
                return;
            }
            if (!outIndices) {
                RC_DBG_ERROR("The OUT INDEX DATA passed to the create index array buffer function is 0.");
                return;
            }
            if (!outInitData) {
                RC_DBG_ERROR("The OUT SUBRESOURCE DATA passed to the create index array buffer function is 0.");
                return;
            }
            if (inIndices.empty()) {
                RC_DBG_ERROR("The IN INDEX DATA passed to the create index array buffer function is empty.");
                return;
            }
        );

        HRESULT hr;

        // Be sure this happens before the index array buffer is created
        *outIndices = inIndices;

        D3D11_BUFFER_DESC vertexArrayBufferDescription = {};
        vertexArrayBufferDescription.Usage             = D3D11_USAGE_DEFAULT;
        vertexArrayBufferDescription.ByteWidth         = outIndices->size() * sizeof(DWORD);
        vertexArrayBufferDescription.BindFlags         = D3D11_BIND_INDEX_BUFFER;
        vertexArrayBufferDescription.CPUAccessFlags    = NULL;

        // Set the data for the index array buffer
        outInitData->pSysMem = outIndices->data();
        hr = this->device->CreateBuffer(&vertexArrayBufferDescription, outInitData, outIndexArrayBuffer->GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create index array buffer");
    }

    void createInputLayout(Microsoft::WRL::ComPtr<ID3D11InputLayout>* outInputLayout, 
                           Microsoft::WRL::ComPtr<ID3DBlob>           inVertexShaderBlob, 
                           Microsoft::WRL::ComPtr<ID3DBlob>           inPixelShaderBlob)
    {
        // Set asserts for debug builds
        RC_DBG_CODE(
			if (!outInputLayout) {
				RC_DBG_ERROR("The OUT INPUT LAYOUT passed to the create input layout function is 0.");
				return;
			}
        );

        HRESULT hr;

        hr = this->device->CreateInputLayout(this->layout, ARRAYSIZE(this->layout),
                                             inVertexShaderBlob->GetBufferPointer(), inVertexShaderBlob->GetBufferSize(),
                                             outInputLayout->GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create input layout");
    }

    void clearScreen() {
        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode              = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode              = D3D11_CULL_NONE;

        Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState = nullptr;
        device->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf());
        context->RSSetState(rasterizerState.Get());

        this->context->ClearRenderTargetView(this->renderTargetView.Get(), DirectX::Colors::CadetBlue);
        this->context->ClearDepthStencilView(this->depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    void createTexture2D(Microsoft::WRL::ComPtr<ID3D11Texture2D>*                      outTexture,
                         std::unique_ptr<unsigned char[], decltype(&stbi_image_free)>* outImageData,
                         const char*                                                   path, 
                         const DXGI_FORMAT                                             format)
    {
		// Set asserts for debug builds
        RC_DBG_CODE(
			if (!outTexture) {
				RC_DBG_ERROR("The OUT TEXTURE passed to the create texture 2D function is 0.");
				return;
			}
		    if (!outImageData) {
			    RC_DBG_ERROR("The OUT IMAGE DATA passed to the create texture 2D function is 0.");
			    return;
		    }
		    if (!path) {
			    RC_DBG_ERROR("The PATH passed to the create texture 2D function is 0.");
			    return;
		    }
        );

        HRESULT hr;

        int width;
        int height;
        int channels;

		// Create an unique pointer to the image data
        std::unique_ptr<unsigned char[], decltype(&stbi_image_free)> imageData(
            stbi_load(path, &width, &height, &channels, STBI_rgb_alpha), stbi_image_free
        );
        RC_EI_ASSERT(!imageData, "Failed to load image data.");

        // Create image description
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

        // Create image init data
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem                = imageData.get();
        initData.SysMemPitch            = width * 4;

        // Create texture
        hr = this->device->CreateTexture2D(&textureDesc, &initData, outTexture->GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create texture 2D.");
        
		// Pass the image data to the output parameter
        *outImageData = std::move(imageData);
    }
    ID3D11Texture2D* getTextureFromShaderResourceView(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> inShaderResourceView) {
        HRESULT hr;

        ID3D11Resource* resource;
        inShaderResourceView->GetResource(&resource);

        ID3D11Texture2D* texture;

        hr = resource->QueryInterface(_uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture));
        RC_EI_ASSERT(FAILED(hr), "Failed to get texture 2D from shader resource view.");

        return texture;
    }

    void createShaderResourceView(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* outShaderResourceView,
                                  Microsoft::WRL::ComPtr<ID3D11Resource>            inResource) 
    {
        RC_DBG_CODE(
            if (!outShaderResourceView) {
                RC_DBG_ERROR("The OUT SHADER RESOURCE VIEW passed to the create shader resource view function is 0.");
                return;
            }
        );

        HRESULT hr;

        hr = this->device->CreateShaderResourceView(inResource.Get(), nullptr, outShaderResourceView->GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create shader resource view.");
    }

    void createSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>* outSamplerState) {
        RC_DBG_CODE(
            if (!outSamplerState) {
                RC_DBG_ERROR("The OUT SAMPLER STATE passed to the create sampler state function is 0.");
                return;
            }
        );

        HRESULT hr;

        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // Linear filtering for min/mag/mip
        samplerDesc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;       // Wrap texture coordinates horizontally
        samplerDesc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;       // Wrap texture coordinates vertically
        samplerDesc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.ComparisonFunc     = D3D11_COMPARISON_NEVER;
        samplerDesc.MinLOD             = 0;
        samplerDesc.MaxLOD             = D3D11_FLOAT32_MAX;

        hr = this->device->CreateSamplerState(&samplerDesc, outSamplerState->GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create sampler state.");
    }
    void bindSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState> inSamplerState) {
        this->context->PSSetSamplers(0, 1, inSamplerState.GetAddressOf());
    }

    void render(Microsoft::WRL::ComPtr<ID3D11VertexShader> inVertexShader, 
                Microsoft::WRL::ComPtr<ID3D11PixelShader>  inPixelShader,
                Microsoft::WRL::ComPtr<ID3D11InputLayout>  inInputLayout,
                Microsoft::WRL::ComPtr<ID3D11Buffer>       inVertexArrayBuffer,
                Microsoft::WRL::ComPtr<ID3D11Buffer>       inIndexArrayBuffer,
                const uint32_t                             inIndexCount) 
    {
        uint32_t stride = sizeof(Vertex);
        uint32_t offset = 0;
        this->context->VSSetShader(inVertexShader.Get(), nullptr, 0);
        this->context->PSSetShader(inPixelShader.Get(), nullptr, 0);

        // Set vertex and index array buffers
        this->context->IASetVertexBuffers(0, 1, inVertexArrayBuffer.GetAddressOf(), &stride, &offset);
        this->context->IASetIndexBuffer(inIndexArrayBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

        // Set input layout and primitive topology
        this->context->IASetInputLayout(inInputLayout.Get());
        this->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        this->context->DrawIndexed(inIndexCount, 0, 0);
    }

    void present() {
        this->swapChain->Present(vSync, 0);
    }

    void setViewport(const uint32_t width, const uint32_t height) {
        viewport.Width    = width;
        viewport.Height   = height;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        this->context->RSSetViewports(1, &this->viewport);
    }
    
    template <typename StructType>
    void createConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>* outBuffer, const StructType& inData) {
        HRESULT hr;

        D3D11_BUFFER_DESC cbufferDesc   = {};
        cbufferDesc.Usage               = D3D11_USAGE_DYNAMIC;
        cbufferDesc.ByteWidth           = sizeof(StructType);
        cbufferDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
        cbufferDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
        cbufferDesc.MiscFlags           = 0;
        cbufferDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.SysMemPitch            = 0;
        initData.SysMemSlicePitch       = 0;
        initData.pSysMem                = &inData;

        hr = this->device->CreateBuffer(&cbufferDesc, &initData, outBuffer->GetAddressOf());
        RC_EI_ASSERT(FAILED(hr), "Failed to create constant buffer.");
    }
    template <typename StructType>
    void updateConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> inBuffer, const StructType& inData) {
        D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		this->context->Map(inBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

		memcpy(mappedResource.pData, &inData, sizeof(StructType));

		context->Unmap(inBuffer.Get(), 0);
    }

    void VSBindBuffers(std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>>& buffers) {
        if (buffers.empty()) return;
        context->VSSetConstantBuffers(0, buffers.size(), buffers.data()->GetAddressOf());
    }
    void PSBindBuffers(std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>>& buffers) {
        if (buffers.empty()) return;
        context->PSSetConstantBuffers(0, buffers.size(), buffers.data()->GetAddressOf());
    }
    
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> getDeviceContext() { return this->context.Get(); }
    Microsoft::WRL::ComPtr<IDXGISwapChain> getSwapChain() { return this->swapChain.Get(); }
    Microsoft::WRL::ComPtr<ID3D11Device> getDevice() { return this->device.Get(); }
};