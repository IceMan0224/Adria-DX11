#pragma once
#include <vector>
#include <d3d11.h>
#include <wrl.h>
#include "../Core/Definitions.h" 
#include "../Core/Macros.h" 


namespace adria
{

	class VertexBuffer
	{
	public:
		VertexBuffer() = default;

		template<typename vertex_t>
		void Create(ID3D11Device* device, vertex_t const* vertices, u32 vertices_count, bool dynamic = false)
		{

			D3D11_BUFFER_DESC vbd = {};
			vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vbd.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
			vbd.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0u;
			vbd.MiscFlags = 0u;
			vbd.ByteWidth = static_cast<u32>(sizeof(vertex_t) * vertices_count);
			vbd.StructureByteStride = sizeof(vertex_t);

			D3D11_SUBRESOURCE_DATA data{};
			data.pSysMem = (void const*)vertices;

			HRESULT hr = device->CreateBuffer(&vbd, &data, buffer.GetAddressOf());
			BREAK_IF_FAILED(hr);
			this->stride = sizeof(vertex_t);
			this->vertex_count = static_cast<u32>(vertices_count);

		}

		template<typename vertex_t>
		void Create(ID3D11Device* device, std::vector<vertex_t> const& vertices, bool dynamic = false)
		{
			Create(device, vertices.data(), (u32)vertices.size(), dynamic);
		}

		void Bind(ID3D11DeviceContext* context, u32 slot, u32 offset = 0u) const
		{
			context->IASetVertexBuffers(slot, 1u, buffer.GetAddressOf(), &stride, &offset);
		}

		template<typename vertex_t>
		void Update(ID3D11DeviceContext* context, std::vector<vertex_t> const& vertices)
		{

			ADRIA_ASSERT(sizeof(vertex_t) == stride);

			Update(context, (void*)vertices.data(), vertices.size() * sizeof(vertex_t));
		}

		void Update(ID3D11DeviceContext* context, void const* p_data, u64 data_size)
		{
			D3D11_BUFFER_DESC buffer_desc;
			buffer->GetDesc(&buffer_desc);
			ADRIA_ASSERT(buffer_desc.Usage == D3D11_USAGE_DYNAMIC && buffer_desc.CPUAccessFlags == D3D11_CPU_ACCESS_WRITE);

			D3D11_MAPPED_SUBRESOURCE mappedResource;
			void* dataPtr;
			context->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			dataPtr = mappedResource.pData;
			memcpy(dataPtr, p_data, data_size);
			context->Unmap(buffer.Get(), 0);
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
		u32 stride;
		u32 vertex_count;

	};


}