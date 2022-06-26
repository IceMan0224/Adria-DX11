#include <unordered_map>
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NOEXCEPTION
#include "tiny_gltf.h"

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

#include "ModelImporter.h"
#include "../tecs/registry.h"
#include "../Graphics/GraphicsDeviceDX11.h"
#include "../Graphics/VertexTypes.h"
#include "../Graphics/TextureManager.h"
#include "../Logging/Logger.h"
#include "../Math/BoundingVolumeHelpers.h"
#include "../Math/ComputeTangentFrame.h"
#include "../Utilities/FilesUtil.h"
#include "../Utilities/Random.h"
#include "../Utilities/Heightmap.h"
#include "../Utilities/Image.h"
#include "../Utilities/HashMap.h"

using namespace DirectX;
namespace adria 
{
    namespace
    {
		void GenerateTerrainLayerTexture(char const* texture_name, Terrain* terrain, TerrainTextureLayerParameters const& params)
		{
			auto [width, depth] = terrain->TileCounts();
			auto [tile_size_x, tile_size_z] = terrain->TileSizes();

			std::vector<BYTE> temp_layer_data(width * depth * 4);
			std::vector<BYTE> layer_data(width * depth * 4);
			for (uint64 j = 0; j < depth; ++j)
			{
				for (uint64 i = 0; i < width; ++i)
				{
					float32 x = i * tile_size_x;
					float32 z = j * tile_size_z;

					float32 height = terrain->HeightAt(x, z);
					float32 normal_y = terrain->NormalAt(x, z).y;

					if (height > params.terrain_rocks_start)
					{
						float32 mix_multiplier = std::max(
							(height - params.terrain_rocks_start) / params.height_mix_zone,
							1.0f
						);
						float32 rock_slope_multiplier = std::clamp((normal_y - params.terrain_slope_rocks_start) / params.slope_mix_zone, 0.0f, 1.0f);
						temp_layer_data[(j * width + i) * 4 + 0] = (BYTE) (BYTE_MAX * mix_multiplier * rock_slope_multiplier);
					}

					if (height > params.terrain_sand_start && height <= params.terrain_sand_end)
					{
                        float32 mix_multiplier = std::min(
                            (height-params.terrain_sand_start) / params.height_mix_zone,
                            (params.terrain_sand_end - height) / params.height_mix_zone
						);
						temp_layer_data[(j * width + i) * 4 + 1] = (BYTE) (BYTE_MAX * mix_multiplier);
					}

					if (height > params.terrain_grass_start && height <= params.terrain_grass_end)
					{
						float32 mix_multiplier = std::min(
							(height - params.terrain_grass_start) / params.height_mix_zone,
							(params.terrain_grass_end - height) / params.height_mix_zone
						);

						float32 grass_slope_multiplier = std::clamp((normal_y - params.terrain_slope_grass_start) / params.slope_mix_zone, 0.0f, 1.0f);
                        temp_layer_data[(j * width + i) * 4 + 2] = (BYTE)(BYTE_MAX * mix_multiplier * grass_slope_multiplier);
					}

					uint32 sum = temp_layer_data[(j * width + i) * 4 + 0]
						+ temp_layer_data[(j * width + i) * 4 + 1] + temp_layer_data[(j * width + i) * 4 + 2] + 1;

					temp_layer_data[(j * width + i) * 4 + 0] = (BYTE)((temp_layer_data[(j * width + i) * 4 + 0] * 1.0f / sum) * BYTE_MAX);
					temp_layer_data[(j * width + i) * 4 + 1] = (BYTE)((temp_layer_data[(j * width + i) * 4 + 1] * 1.0f / sum) * BYTE_MAX);
					temp_layer_data[(j * width + i) * 4 + 2] = (BYTE)((temp_layer_data[(j * width + i) * 4 + 2] * 1.0f / sum) * BYTE_MAX);
				}
			}

			layer_data = temp_layer_data;

			for (size_t j = 2; j < depth - 2; ++j)
			{
				for (size_t i = 2; i < width - 2; ++i)
				{
					int32 n1 = 0, n2 = 0, n3 = 0, n4 = 0;
					for (int32 k = -2; k <= 2; ++k)
					{
						for (int32 l = -2; l <= 2; ++l)
						{
							n1 += (int32)temp_layer_data[((j + k) * width + i + l) * 4 + 0];
							n2 += (int32)temp_layer_data[((j + k) * width + i + l) * 4 + 1];
							n3 += (int32)temp_layer_data[((j + k) * width + i + l) * 4 + 2];
						}
					}
            
					layer_data[(j * width + i) * 4 + 0] = (BYTE)(n1 / 25);
					layer_data[(j * width + i) * 4 + 1] = (BYTE)(n2 / 25);
					layer_data[(j * width + i) * 4 + 2] = (BYTE)(n3 / 25);
				}
			}

			WriteImageTGA(texture_name, layer_data, (int32)width, (int32)depth);
		}
    }

    using namespace tecs;

    std::vector<entity> ModelImporter::LoadGrid(GridParameters const& params, std::vector<TexturedNormalVertex>* vertices_out)
    {
        if (params.heightmap)
        {
            ADRIA_ASSERT(params.heightmap->Depth() == params.tile_count_z + 1);
            ADRIA_ASSERT(params.heightmap->Width() == params.tile_count_x + 1);
        }

        std::vector<entity> chunks;
        std::vector<TexturedNormalVertex> vertices{};
        for (uint64 j = 0; j <= params.tile_count_z; j++)
        {
            for (uint64 i = 0; i <= params.tile_count_x; i++)
            {
                TexturedNormalVertex vertex{};

                float32 height = params.heightmap ? params.heightmap->HeightAt(i, j) : 0.0f;

                vertex.position = XMFLOAT3(i * params.tile_size_x + params.grid_offset.x, 
                    height + params.grid_offset.y, j * params.tile_size_z + params.grid_offset.z);
                vertex.uv = XMFLOAT2(i * 1.0f * params.texture_scale_x / (params.tile_count_x - 1), j * 1.0f * params.texture_scale_z / (params.tile_count_z - 1));
                vertex.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
                vertices.push_back(vertex);
            }
        }

        if (!params.split_to_chunks)
        {
            std::vector<uint32> indices{};
            uint32 i1 = 0;
            uint32 i2 = 1;
            uint32 i3 = static_cast<uint32>(i1 + params.tile_count_x + 1);
            uint32 i4 = static_cast<uint32>(i2 + params.tile_count_x + 1);
            for (uint64 i = 0; i < params.tile_count_x * params.tile_count_z; ++i)
            {
                indices.push_back(i1);
                indices.push_back(i3);
                indices.push_back(i2);


                indices.push_back(i2);
                indices.push_back(i3);
                indices.push_back(i4);

                ++i1;
                ++i2;
                ++i3;
                ++i4;

                if (i1 % (params.tile_count_x + 1) == params.tile_count_x)
                {
                    ++i1;
                    ++i2;
                    ++i3;
                    ++i4;
                }
            }
            ComputeNormals(params.normal_type, vertices, indices);

            entity grid = reg.create();
            Mesh mesh{};
			mesh.vertex_buffer = std::make_shared<Buffer>(gfx, VertexBufferDesc(vertices.size(), sizeof(TexturedNormalVertex)), vertices.data());
			mesh.index_buffer = std::make_shared<Buffer>(gfx, IndexBufferDesc(indices.size(), false), indices.data());
            mesh.indices_count = (uint32)indices.size();
 
            reg.emplace<Mesh>(grid, mesh);
            reg.emplace<Transform>(grid);

            BoundingBox aabb = AABBFromRange(vertices.begin(), vertices.end());
            reg.emplace<Visibility>(grid, aabb, true, true);

            chunks.push_back(grid);
        }
        else
        {
            std::vector<uint32> indices{};
            for (size_t j = 0; j < params.tile_count_z; j += params.chunk_count_z)
            {
                for (size_t i = 0; i < params.tile_count_x; i += params.chunk_count_x)
                {
                    entity chunk = reg.create();
                    uint32 const indices_count = static_cast<uint32>(params.chunk_count_z * params.chunk_count_x * 3 * 2);
                    uint32 const indices_offset = static_cast<uint32>(indices.size());
                    std::vector<TexturedNormalVertex> chunk_vertices_aabb{};
                    for (size_t k = j; k < j + params.chunk_count_z; ++k)
                    {
                        for (size_t m = i; m < i + params.chunk_count_x; ++m)
                        {
                            uint32 i1 = static_cast<uint32>(k * (params.tile_count_x + 1) + m);
                            uint32 i2 = static_cast<uint32>(i1 + 1);
                            uint32 i3 = static_cast<uint32>((k + 1) * (params.tile_count_x + 1) + m);
                            uint32 i4 = static_cast<uint32>(i3 + 1);

                            indices.push_back(i1);
                            indices.push_back(i3);
                            indices.push_back(i2);

                            indices.push_back(i2);
                            indices.push_back(i3);
                            indices.push_back(i4);

                            chunk_vertices_aabb.push_back(vertices[i1]);
                            chunk_vertices_aabb.push_back(vertices[i2]);
                            chunk_vertices_aabb.push_back(vertices[i3]);
                            chunk_vertices_aabb.push_back(vertices[i4]);
                        }
                    }
                    Mesh mesh{};
                    mesh.indices_count = indices_count;
                    mesh.start_index_location = indices_offset;

                    reg.emplace<Mesh>(chunk, mesh);
                    reg.emplace<Transform>(chunk);
                    BoundingBox aabb = AABBFromRange(chunk_vertices_aabb.begin(), chunk_vertices_aabb.end());
                    reg.emplace<Visibility>(chunk, aabb, true, true);
                    chunks.push_back(chunk);
                }
            }
            ComputeNormals(params.normal_type, vertices, indices);
			std::shared_ptr<Buffer> vb = std::make_shared<Buffer>(gfx, VertexBufferDesc(vertices.size(), sizeof(TexturedNormalVertex)), vertices.data());
			std::shared_ptr<Buffer> ib = std::make_shared<Buffer>(gfx, IndexBufferDesc(indices.size(), false), indices.data());
            for (entity chunk : chunks)
            {
                auto& mesh = reg.get<Mesh>(chunk);

                mesh.vertex_buffer = vb;
                mesh.index_buffer = ib;
            }
        }

        if (vertices_out) *vertices_out = vertices;
        return chunks;
    }

	std::vector<entity> ModelImporter::LoadObjMesh(std::string const& model_path, std::vector<std::string>* diffuse_textures_out)
	{
		tinyobj::ObjReaderConfig reader_config{};
		tinyobj::ObjReader reader;
		std::string model_name = GetFilename(model_path);
		if (!reader.ParseFromFile(model_path, reader_config))
		{
            if (!reader.Error().empty())
            {
                ADRIA_LOG(ERROR, reader.Error().c_str());
            }
			return {};
		}
		if (!reader.Warning().empty())
		{
			ADRIA_LOG(WARNING, reader.Warning().c_str());
		}
		tinyobj::attrib_t const& attrib = reader.GetAttrib();
		std::vector<tinyobj::shape_t> const& shapes = reader.GetShapes();
        std::vector<tinyobj::material_t> const& materials = reader.GetMaterials();
		std::vector<entity> entities{};
        std::vector<std::string> diffuse_textures;
		for (size_t s = 0; s < shapes.size(); s++)
		{
            std::vector<TexturedNormalVertex> vertices{};

			entity e = reg.create();
			entities.push_back(e);

			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

				for (size_t v = 0; v < fv; v++)
				{
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

					TexturedNormalVertex vertex{};
					tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
					tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
					tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

					vertex.position.x = vx;
					vertex.position.y = vy;
					vertex.position.z = vz;

					// Check if `normal_index` is zero or positive. negative = no normal data
					if (idx.normal_index >= 0)
					{
						tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
						tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
						tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

						vertex.normal.x = nx;
						vertex.normal.y = ny;
						vertex.normal.z = nz;
					}

					// Check if `texcoord_index` is zero or positive. negative = no texcoord data
					if (idx.texcoord_index >= 0)
					{
						tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
						tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

						vertex.uv.x = tx;
						vertex.uv.y = ty;
					}

					vertices.push_back(vertex);
				}

                index_offset += fv;
			}

			std::shared_ptr<Buffer> vb = std::make_shared<Buffer>(gfx, VertexBufferDesc(vertices.size(), sizeof(TexturedNormalVertex)), vertices.data());

			Mesh mesh_component{};
			mesh_component.base_vertex_location = 0;
			mesh_component.vertex_count = static_cast<uint32>(vertices.size());
            mesh_component.vertex_buffer = vb;
			reg.emplace<Mesh>(e, mesh_component);

			reg.emplace<Tag>(e, model_name + " mesh" + std::to_string(as_integer(e)));

			if (diffuse_textures_out)
			{
				ADRIA_ASSERT(shapes[s].mesh.material_ids.size() > 0);
				int material_id = shapes[s].mesh.material_ids[0];
				ADRIA_ASSERT(material_id >= 0);
				tinyobj::material_t material = materials[material_id];
				diffuse_textures_out->push_back(material.diffuse_texname);
			}

		}
        ADRIA_LOG(INFO, "OBJ Mesh %s successfully loaded!", model_path.c_str());
		return entities;
	}

	ModelImporter::ModelImporter(registry& reg, GraphicsDevice* gfx, TextureManager& texture_manager) : reg(reg),
        texture_manager(texture_manager), gfx(gfx)
    {}

	std::vector<entity> ModelImporter::ImportModel_GLTF(ModelParameters const& params)
	{
		tinygltf::TinyGLTF loader;
		tinygltf::Model model;
		std::string err;
		std::string warn;
		bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, params.model_path);

		std::string model_name = GetFilename(params.model_path);
		if (!warn.empty())
		{
			ADRIA_LOG(WARNING, warn.c_str());
		}
		if (!err.empty())
		{
			ADRIA_LOG(ERROR, err.c_str());
			return {};
		}
		if (!ret)
		{
			ADRIA_LOG(ERROR, "Failed to load model %s", model_name.c_str());
			return {};
		}

		std::vector<CompleteVertex> vertices{};
		std::vector<uint32> indices{};
		std::vector<entity> entities{};
		HashMap<std::string, std::vector<entity>> mesh_name_to_entities_map;
		for (auto& mesh : model.meshes)
		{
			std::vector<entity>& mesh_entities = mesh_name_to_entities_map[mesh.name];
			for (auto& primitive : mesh.primitives)
			{
				ADRIA_ASSERT(primitive.indices >= 0);
				tinygltf::Accessor const& index_accessor = model.accessors[primitive.indices];

				entity e = reg.create();
				entities.push_back(e);
				mesh_entities.push_back(e);

				Material material{};
				tinygltf::Material gltf_material = model.materials[primitive.material];
				tinygltf::PbrMetallicRoughness pbr_metallic_roughness = gltf_material.pbrMetallicRoughness;
				if (pbr_metallic_roughness.baseColorTexture.index >= 0)
				{
					tinygltf::Texture const& base_texture = model.textures[pbr_metallic_roughness.baseColorTexture.index];
					tinygltf::Image const& base_image = model.images[base_texture.source];
					std::string texbase = params.textures_path + base_image.uri;
					material.albedo_texture = texture_manager.LoadTexture(ConvertToWide(texbase));
					material.albedo_factor = (float32)pbr_metallic_roughness.baseColorFactor[0];
				}
				if (pbr_metallic_roughness.metallicRoughnessTexture.index >= 0)
				{
					tinygltf::Texture const& metallic_roughness_texture = model.textures[pbr_metallic_roughness.metallicRoughnessTexture.index];
					tinygltf::Image const& metallic_roughness_image = model.images[metallic_roughness_texture.source];
					std::string texmetallicroughness = params.textures_path + metallic_roughness_image.uri;
					material.metallic_roughness_texture = texture_manager.LoadTexture(ConvertToWide(texmetallicroughness));
					material.metallic_factor = (float32)pbr_metallic_roughness.metallicFactor;
					material.roughness_factor = (float32)pbr_metallic_roughness.roughnessFactor;
				}
				if (gltf_material.normalTexture.index >= 0)
				{
					tinygltf::Texture const& normal_texture = model.textures[gltf_material.normalTexture.index];
					tinygltf::Image const& normal_image = model.images[normal_texture.source];
					std::string texnormal = params.textures_path + normal_image.uri;
					material.normal_texture = texture_manager.LoadTexture(ConvertToWide(texnormal));
				}
				if (gltf_material.emissiveTexture.index >= 0)
				{
					tinygltf::Texture const& emissive_texture = model.textures[gltf_material.emissiveTexture.index];
					tinygltf::Image const& emissive_image = model.images[emissive_texture.source];
					std::string texemissive = params.textures_path + emissive_image.uri;
					material.emissive_texture = texture_manager.LoadTexture(ConvertToWide(texemissive));
					material.emissive_factor = (float32)gltf_material.emissiveFactor[0];
				}
				material.shader = EShaderProgram::GBufferPBR;
				material.alpha_cutoff = gltf_material.alphaCutoff;
				material.double_sided = gltf_material.doubleSided;
				if (gltf_material.alphaMode == "OPAQUE")
				{
					material.alpha_mode = EMaterialAlphaMode::Opaque;
					material.shader = EShaderProgram::GBufferPBR;
				}
				else if (gltf_material.alphaMode == "BLEND")
				{
					material.alpha_mode = EMaterialAlphaMode::Blend;
				}
				else if (gltf_material.alphaMode == "MASK")
				{
					material.alpha_mode = EMaterialAlphaMode::Mask;
					material.shader = EShaderProgram::GBufferPBR_Mask;
				}

				reg.emplace<Material>(e, material);
				reg.emplace<Deferred>(e);

				Mesh mesh_component{};
				mesh_component.indices_count = static_cast<uint32>(index_accessor.count);
				mesh_component.start_index_location = static_cast<uint32>(indices.size());
				mesh_component.base_vertex_location = static_cast<uint32>(vertices.size());
				switch (primitive.mode)
				{
				case TINYGLTF_MODE_POINTS:
					mesh_component.topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
					break;
				case TINYGLTF_MODE_LINE:
					mesh_component.topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
					break;
				case TINYGLTF_MODE_LINE_STRIP:
					mesh_component.topology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
					break;
				case TINYGLTF_MODE_TRIANGLES:
					mesh_component.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					break;
				case TINYGLTF_MODE_TRIANGLE_STRIP:
					mesh_component.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
					break;
				default:
					assert(false);
				}

				tinygltf::Accessor const& accessor = model.accessors[primitive.indices];
				tinygltf::BufferView const& bufferView = model.bufferViews[accessor.bufferView];
				tinygltf::Buffer const& buffer = model.buffers[bufferView.buffer];

				int stride = accessor.ByteStride(bufferView);
				uint32 vertex_offset = mesh_component.base_vertex_location;
				uint32 index_count = mesh_component.indices_count;
				uint32 index_offset = mesh_component.start_index_location;
				indices.reserve(indices.size() + index_count);
				unsigned char const* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;
				if (stride == 1)
				{
					for (size_t i = 0; i < index_count; i += 3)
					{
						indices.push_back(data[i + 0]);
						indices.push_back(data[i + 1]);
						indices.push_back(data[i + 2]);
					}
				}
				else if (stride == 2)
				{
					for (size_t i = 0; i < index_count; i += 3)
					{
						indices.push_back(((uint16_t*)data)[i + 0]);
						indices.push_back(((uint16_t*)data)[i + 1]);
						indices.push_back(((uint16_t*)data)[i + 2]);
					}
				}
				else if (stride == 4)
				{
					for (size_t i = 0; i < index_count; i += 3)
					{
						indices.push_back(((uint32_t*)data)[i + 0]);
						indices.push_back(((uint32_t*)data)[i + 1]);
						indices.push_back(((uint32_t*)data)[i + 2]);
					}
				}
				else ADRIA_ASSERT(false);

				std::vector<XMFLOAT3> positions;
				std::vector<XMFLOAT3> normals;
				std::vector<XMFLOAT2> uvs;
				std::vector<XMFLOAT3> tangents;
				std::vector<float>    tangent_handness;
				std::vector<XMFLOAT3> bitangents;
				for (auto& attr : primitive.attributes)
				{
					std::string const& attr_name = attr.first;
					int attr_data = attr.second;

					const tinygltf::Accessor& accessor = model.accessors[attr_data];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

					int stride = accessor.ByteStride(bufferView);
					size_t vertex_count = accessor.count;
					const unsigned char* data = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;

					if (!attr_name.compare("POSITION"))
					{
						positions.reserve(vertex_count);
						for (size_t i = 0; i < vertex_count; ++i)
						{
							positions.push_back(*(XMFLOAT3*)((size_t)data + i * stride));
						}
					}
					else if (!attr_name.compare("NORMAL"))
					{
						normals.reserve(vertex_count);
						for (size_t i = 0; i < vertex_count; ++i)
						{
							normals.push_back(*(XMFLOAT3*)((size_t)data + i * stride));
							if (material.double_sided)
							{
								normals.back().x *= -1;
								normals.back().y *= -1;
								normals.back().z *= -1;
							}
						}
					}
					else if (!attr_name.compare("TANGENT"))
					{
						tangents.reserve(vertex_count);
						for (size_t i = 0; i < vertex_count; ++i)
						{
							XMFLOAT4 tangent = *(XMFLOAT4*)((size_t)data + i * stride);
							tangents.emplace_back(tangent.x, tangent.y, tangent.z);
							tangent_handness.push_back(tangent.w);
						}
					}
					else if (!attr_name.compare("TEXCOORD_0"))
					{
						uvs.reserve(vertex_count);
						if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
						{
							for (size_t i = 0; i < vertex_count; ++i)
							{
								XMFLOAT2 tex = *(XMFLOAT2*)((size_t)data + i * stride);
								tex.y = 1.0f - tex.y;
								uvs.push_back(tex);
							}
						}
						else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
						{
							for (size_t i = 0; i < vertex_count; ++i)
							{
								uint8 const& s = *(uint8*)((size_t)data + i * stride + 0 * sizeof(uint8));
								uint8 const& t = *(uint8*)((size_t)data + i * stride + 1 * sizeof(uint8));
								uvs.push_back(XMFLOAT2(s / 255.0f, 1.0f - t / 255.0f));
							}
						}
						else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
						{
							for (size_t i = 0; i < vertex_count; ++i)
							{
								uint16_t const& s = *(uint16*)((size_t)data + i * stride + 0 * sizeof(uint16));
								uint16_t const& t = *(uint16*)((size_t)data + i * stride + 1 * sizeof(uint16));
								uvs.push_back(XMFLOAT2(s / 65535.0f, 1.0f - t / 65535.0f));
							}
						}
					}
				}
				bool has_tangents = !tangents.empty();
				ADRIA_ASSERT(tangent_handness.size() == tangents.size());
				size_t vertex_count = positions.size();
				if (normals.size() != vertex_count) normals.resize(vertex_count);
				if (uvs.size() != vertex_count) uvs.resize(vertex_count);
				if (tangents.size() != vertex_count) tangents.resize(vertex_count);
				if (bitangents.size() != vertex_count) bitangents.resize(vertex_count);

				mesh_component.vertex_count = vertex_count;
				reg.emplace<Mesh>(e, mesh_component);
				if (has_tangents)
				{
					for (size_t i = 0; i < vertex_count; ++i)
					{
						float tangent_w = tangent_handness[i];
						XMVECTOR _bitangent = XMVectorScale(XMVector3Cross(XMLoadFloat3(&normals[i]), XMLoadFloat3(&tangents[i])), tangent_w);
						XMStoreFloat3(&bitangents[i], XMVector3Normalize(_bitangent));
					}
				}
				else
				{
					//ComputeTangentFrame(indices.data() + index_offset - index_count, index_count,
					//	positions.data(), normals.data(), uvs.data(), vertex_count,
					//	tangents.data(), bitangents.data());
				}

				vertices.reserve(vertices.size() + vertex_count);
				for (size_t i = 0; i < vertex_count; ++i)
				{
					vertices.emplace_back(
						positions[i],
						uvs[i],
						normals[i],
						XMFLOAT3(tangents[i].x, tangents[i].y, tangents[i].z),
						bitangents[i]
					);
				}
			}
		}

		std::function<void(int, XMMATRIX)> LoadNode;
		LoadNode = [&](int node_index, XMMATRIX parent_transform)
		{
			if (node_index < 0) return;
			auto& node = model.nodes[node_index];
			struct Transforms
			{
				XMFLOAT4 rotation_local = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
				XMFLOAT3 scale_local = XMFLOAT3(1.0f, 1.0f, 1.0f);
				XMFLOAT3 translation_local = XMFLOAT3(0.0f, 0.0f, 0.0f);
				XMFLOAT4X4 world;
				bool update = true;
				void Update()
				{
					if (update)
					{
						XMVECTOR S_local = XMLoadFloat3(&scale_local);
						XMVECTOR R_local = XMLoadFloat4(&rotation_local);
						XMVECTOR T_local = XMLoadFloat3(&translation_local);
						XMMATRIX WORLD = XMMatrixScalingFromVector(S_local) *
							XMMatrixRotationQuaternion(R_local) *
							XMMatrixTranslationFromVector(T_local);
						XMStoreFloat4x4(&world, WORLD);
					}
				}
			} transforms;

			if (!node.scale.empty())
			{
				transforms.scale_local = XMFLOAT3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]);
			}
			if (!node.rotation.empty())
			{
				transforms.rotation_local = XMFLOAT4((float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3]);
			}
			if (!node.translation.empty())
			{
				transforms.translation_local = XMFLOAT3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]);
			}
			if (!node.matrix.empty())
			{
				transforms.world._11 = (float)node.matrix[0];
				transforms.world._12 = (float)node.matrix[1];
				transforms.world._13 = (float)node.matrix[2];
				transforms.world._14 = (float)node.matrix[3];
				transforms.world._21 = (float)node.matrix[4];
				transforms.world._22 = (float)node.matrix[5];
				transforms.world._23 = (float)node.matrix[6];
				transforms.world._24 = (float)node.matrix[7];
				transforms.world._31 = (float)node.matrix[8];
				transforms.world._32 = (float)node.matrix[9];
				transforms.world._33 = (float)node.matrix[10];
				transforms.world._34 = (float)node.matrix[11];
				transforms.world._41 = (float)node.matrix[12];
				transforms.world._42 = (float)node.matrix[13];
				transforms.world._43 = (float)node.matrix[14];
				transforms.world._44 = (float)node.matrix[15];
				transforms.update = false;
			}
			transforms.Update();

			if (node.mesh >= 0)
			{
				auto const& mesh = model.meshes[node.mesh];
				std::vector<entity> const& mesh_entities = mesh_name_to_entities_map[mesh.name];
				for (entity e : mesh_entities)
				{
					Mesh const& mesh = reg.get<Mesh>(e);
					XMMATRIX model = XMLoadFloat4x4(&transforms.world) * parent_transform;
					BoundingBox aabb = AABBFromRange(vertices.begin() + mesh.base_vertex_location, vertices.begin() + mesh.base_vertex_location + mesh.vertex_count);
					aabb.Transform(aabb, model);
					reg.emplace<Visibility>(e, aabb, true, true);
					reg.emplace<Transform>(e, model, model);
				}
			}

			for (int child : node.children) LoadNode(child, XMLoadFloat4x4(&transforms.world) * parent_transform);
		};
		tinygltf::Scene const& scene = model.scenes[std::max(0, model.defaultScene)];
		for (size_t i = 0; i < scene.nodes.size(); ++i)
		{
			LoadNode(scene.nodes[i], params.model_matrix);
		}

		std::shared_ptr<Buffer> vb = std::make_shared<Buffer>(gfx, VertexBufferDesc(vertices.size(), sizeof(CompleteVertex)), vertices.data());
		std::shared_ptr<Buffer> ib = std::make_shared<Buffer>(gfx, IndexBufferDesc(indices.size(), false), indices.data());

		for (entity e : entities)
		{
			auto& mesh = reg.get<Mesh>(e);
			mesh.vertex_buffer = vb;
			mesh.index_buffer = ib;
			reg.emplace<Tag>(e, model_name + " mesh" + std::to_string(as_integer(e)));
		}
		ADRIA_LOG(INFO, "GLTF Mesh %s successfully loaded!", params.model_path.c_str());
		return entities;
	}

    entity ModelImporter::LoadSkybox(SkyboxParameters const& params)
    {
        entity skybox = reg.create();

        Skybox sky{};
        sky.active = true;

        if (params.cubemap.has_value()) sky.cubemap_texture = texture_manager.LoadCubeMap(params.cubemap.value());
        else sky.cubemap_texture = texture_manager.LoadCubeMap(params.cubemap_textures);

        reg.emplace<Skybox>(skybox, sky);
        reg.emplace<Tag>(skybox, "Skybox");
        
        return skybox;
    }

    entity ModelImporter::LoadLight(LightParameters const& params)
    {
        entity light = reg.create();

        if (params.light_data.type == ELightType::Directional)
            const_cast<LightParameters&>(params).light_data.position = XMVectorScale(-params.light_data.direction, 1e3);
  
        reg.emplace<Light>(light, params.light_data);

        if (params.mesh_type == ELightMesh::Quad)
        {
            uint32 const size = params.mesh_size;
            std::vector<TexturedVertex> const vertices =
            {
                { {-0.5f * size, -0.5f * size, 0.0f}, {0.0f, 0.0f}},
                { { 0.5f * size, -0.5f * size, 0.0f}, {1.0f, 0.0f}},
                { { 0.5f * size,  0.5f * size, 0.0f}, {1.0f, 1.0f}},
                { {-0.5f * size,  0.5f * size, 0.0f}, {0.0f, 1.0f}}
            };
            std::vector<uint16> const indices =
            {
                    0, 2, 1, 2, 0, 3
            };

			Mesh mesh{};
			mesh.vertex_buffer = std::make_shared<Buffer>(gfx, VertexBufferDesc(vertices.size(), sizeof(TexturedVertex)), vertices.data());
			mesh.index_buffer = std::make_shared<Buffer>(gfx, IndexBufferDesc(indices.size(), true), indices.data());
            mesh.indices_count = static_cast<uint32>(indices.size());

            reg.emplace<Mesh>(light, mesh);

            Material material{};
            XMStoreFloat3(&material.diffuse, params.light_data.color);

            if (params.light_texture.has_value())
                material.albedo_texture = texture_manager.LoadTexture(params.light_texture.value()); //
            else if(params.light_data.type == ELightType::Directional)
                material.albedo_texture = texture_manager.LoadTexture(L"Resources/Textures/sun.png");

            if (params.light_data.type == ELightType::Directional)
                material.shader = EShaderProgram::Sun;
            else if (material.albedo_texture != INVALID_TEXTURE_HANDLE)
                material.shader = EShaderProgram::Billboard;
            else 
            { 
                ADRIA_LOG(ERROR, "Light with quad mesh needs diffuse texture!"); 
            }

            reg.emplace<Material>(light, material); 
            
            BoundingBox aabb = AABBFromRange(vertices.begin(), vertices.end());
            auto translation_matrix = XMMatrixTranslationFromVector(params.light_data.position);
            aabb.Transform(aabb, XMMatrixTranslationFromVector(params.light_data.position));

            reg.emplace<Visibility>(light, aabb, true, false);
            reg.emplace<Transform>(light, translation_matrix, translation_matrix);
            //sun rendered in separate pass
            if (params.light_data.type != ELightType::Directional) reg.emplace<Forward>(light, true);
        }
        else if (params.mesh_type == ELightMesh::Cube)
        {
		    
           const SimpleVertex cube_vertices[8] = {
			   XMFLOAT3{ -1.0, -1.0,  1.0 },
			   XMFLOAT3{ 1.0, -1.0,  1.0 },
			   XMFLOAT3{ 1.0,  1.0,  1.0 },
			   XMFLOAT3{ -1.0,  1.0,  1.0 },
			   XMFLOAT3{ -1.0, -1.0, -1.0 },
			   XMFLOAT3{ 1.0, -1.0, -1.0 },
			   XMFLOAT3{ 1.0,  1.0, -1.0 },
			   XMFLOAT3{ -1.0,  1.0, -1.0 }
			};

			const uint16_t cube_indices[36] = {
				// front
				0, 1, 2,
				2, 3, 0,
				// right
				1, 5, 6,
				6, 2, 1,
				// back
				7, 6, 5,
				5, 4, 7,
				// left
				4, 0, 3,
				3, 7, 4,
				// bottom
				4, 5, 1,
				1, 0, 4,
				// top
				3, 2, 6,
				6, 7, 3
			};

			//Mesh skybox_mesh{};
			//skybox_mesh.vb = std::make_shared<VertexBuffer>();
			//skybox_mesh.ib = std::make_shared<IndexBuffer>();
			//skybox_mesh.vb->Create(device, cube_vertices, ARRAYSIZE(cube_vertices));
			//skybox_mesh.ib->Create(device, cube_indices, ARRAYSIZE(cube_indices));
			//skybox_mesh.indices_count = ARRAYSIZE(cube_indices);

			//reg.emplace<Mesh>(skybox, skybox_mesh);

            //load sphere mesh and mesh component
           //Mesh sphere_mesh{};
           //
           //
           //Material material{};
           //XMStoreFloat3(&material.diffuse, params.light_data.color);
           //
           //if (params.light_texture.has_value())
           //    material.diffuse_texture = texture_manager.LoadTexture(params.light_texture.value()); //
           //else if (params.light_data.type == LightType::eDirectional)
           //    material.diffuse_texture = texture_manager.LoadTexture(L"Resources/Textures/sun.png");
           //
           //if (params.light_data.type == LightType::eDirectional)
           //    material.shader = StandardShader::eSun;
           //else if (material.diffuse_texture == INVALID_TEXTURE_HANDLE)
           //    material.shader = StandardShader::eSolid;
           //else material.shader = StandardShader::eTexture;
           //
           //reg.emplace<Material>(light, material);
        }

        switch (params.light_data.type)
        {
        case ELightType::Directional:
            reg.emplace<Tag>(light, "Directional Light");
            break;
        case ELightType::Spot:
            reg.emplace<Tag>(light, "Spot Light");
            break;
        case ELightType::Point:
            reg.emplace<Tag>(light, "Point Light");
            break;
        }

        return light;
	}

    std::vector<entity> ModelImporter::LoadOcean(OceanParameters const& params)
    {
        std::vector<entity> ocean_chunks = ModelImporter::LoadGrid(params.ocean_grid);

        Material ocean_material{};
        ocean_material.diffuse = XMFLOAT3(0.0123f, 0.3613f, 0.6867f); //0, 105, 148
        ocean_material.shader = EShaderProgram::Unknown; 

        Ocean ocean_component{};
        for (auto ocean_chunk : ocean_chunks)
        {
            reg.emplace<Material>(ocean_chunk, ocean_material);
            reg.emplace<Ocean>(ocean_chunk, ocean_component);
            reg.emplace<Tag>(ocean_chunk, "Ocean Chunk" + std::to_string(as_integer(ocean_chunk)));
        }

        return ocean_chunks;
	}

    std::vector<entity> ModelImporter::LoadTerrain(TerrainParameters& params)
	{
        std::vector<TexturedNormalVertex> vertices;
		std::vector<entity> terrain_chunks = LoadGrid(params.terrain_grid, &vertices);

        TerrainComponent::terrain = std::make_unique<Terrain>(vertices,
            params.terrain_grid.tile_size_x,
            params.terrain_grid.tile_size_z, 
            params.terrain_grid.tile_count_x,
            params.terrain_grid.tile_count_z);

        TerrainComponent::texture_scale = XMFLOAT2(params.terrain_grid.texture_scale_x,
            params.terrain_grid.texture_scale_z);

        GenerateTerrainLayerTexture(params.layer_texture.c_str(), TerrainComponent::terrain.get(), params.layer_params);

        TerrainComponent terrain_component{};
		terrain_component.grass_texture = texture_manager.LoadTexture(params.grass_texture);
		terrain_component.rock_texture = texture_manager.LoadTexture(params.rock_texture);
		terrain_component.base_texture = texture_manager.LoadTexture(params.base_texture);
		terrain_component.sand_texture = texture_manager.LoadTexture(params.sand_texture);
        terrain_component.layer_texture = texture_manager.LoadTexture(params.layer_texture);

		for (auto terrain_chunk : terrain_chunks)
		{
			reg.emplace<TerrainComponent>(terrain_chunk, terrain_component);
			reg.emplace<Tag>(terrain_chunk, "Terrain Chunk" + std::to_string(as_integer(terrain_chunk)));
		}

		return terrain_chunks;
	}

	entity ModelImporter::LoadFoliage(FoliageParameters const& params)
	{
		const float32 size = params.foliage_scale;
		
		struct FoliageInstance
		{
			XMFLOAT3 position;
            float32 rotation_y;
		};

		RealRandomGenerator<float> random_x(params.foliage_center.x - params.foliage_extents.x, params.foliage_center.x + params.foliage_extents.x);
		RealRandomGenerator<float> random_z(params.foliage_center.y - params.foliage_extents.y, params.foliage_center.y + params.foliage_extents.y);
        RealRandomGenerator<float> random_angle(0.0f, 2.0f * pi<float32>);

		std::vector<entity> foliages;

		switch (params.mesh_texture_pair.first)
		{
		case EFoliageMesh::SingleQuad:
			foliages = LoadObjMesh("Resources/Models/Foliage/foliagequad_single.obj");
			break;
		case EFoliageMesh::DoubleQuad:
			foliages = LoadObjMesh("Resources/Models/Foliage/foliagequad_double.obj");
			break;
		case EFoliageMesh::TripleQuad:
			foliages = LoadObjMesh("Resources/Models/Foliage/foliagequad_triple.obj");
			break;
		default:
			foliages = LoadObjMesh("Resources/Models/Foliage/foliagequad_single.obj");
			break;
		}
		ADRIA_ASSERT(foliages.size() == 1);
		entity foliage = foliages[0];

		std::vector<FoliageInstance> instance_data{};
		for (int32 i = 0; i < params.foliage_count; ++i)
		{
            static const uint32 MAX_ITERATIONS = 5;
			XMFLOAT3 position{};
			XMFLOAT3 normal{};
			uint32 iteration = 0;
			do
			{
                if (iteration > MAX_ITERATIONS) break;
				position.x = random_x();
				position.z = random_z();
				position.y = TerrainComponent::terrain ? TerrainComponent::terrain->HeightAt(position.x, position.z) - 0.5f : -0.5f;
				normal = TerrainComponent::terrain ? TerrainComponent::terrain->NormalAt(position.x, position.z) : XMFLOAT3(0.0f, 1.0f, 0.0f);

				++iteration;
			} while (position.y > params.foliage_height_end || position.y < params.foliage_height_start || normal.y < params.foliage_slope_start);

			if(iteration < MAX_ITERATIONS) instance_data.emplace_back(position, random_angle());
		}

		auto& mesh_component = reg.get<Mesh>(foliage);
		mesh_component.instance_buffer = std::make_shared<Buffer>(gfx, VertexBufferDesc(instance_data.size(), sizeof(FoliageInstance)), instance_data.data());
		mesh_component.start_instance_location = 0;
		mesh_component.instance_count = (uint32)instance_data.size();

		Material material{};
		material.albedo_texture = texture_manager.LoadTexture(params.mesh_texture_pair.second);
		material.albedo_factor = 1.0f;
		material.shader = EShaderProgram::GBuffer_Foliage;
		reg.emplace<Material>(foliage, material);
		reg.emplace<Foliage>(foliage);

		Transform transform{};
		transform.starting_transform = XMMatrixScaling(size, size, size);
		transform.current_transform = transform.starting_transform;
		reg.emplace<Transform>(foliage, transform);

		Visibility vis{};
		vis.skip_culling = true;
		reg.emplace<Visibility>(foliage, vis);

		return foliage;
	}

    std::vector<entity> ModelImporter::LoadTrees(TreeParameters const& params)
	{
		const float32 size = params.tree_scale;

		struct TreeInstance
		{
			XMFLOAT3 position;
			float32 rotation_y;
		};

		RealRandomGenerator<float> random_x(params.tree_center.x - params.tree_extents.x, params.tree_center.x + params.tree_extents.x);
		RealRandomGenerator<float> random_z(params.tree_center.y - params.tree_extents.y, params.tree_center.y + params.tree_extents.y);
		RealRandomGenerator<float> random_angle(0.0f, 2.0f * pi<float32>);

        std::vector<std::string> diffuse_textures{};
        std::vector<entity> trees;

        std::string texture_path;
        switch (params.tree_type)
        {
        case ETreeType::Tree01:
            trees = LoadObjMesh("Resources/Models/Trees/Tree01/tree01.obj", &diffuse_textures);
            texture_path = "Resources/Models/Trees/Tree01/";
            break;
        case ETreeType::Tree02:
        default:
            trees = LoadObjMesh("Resources/Models/Trees/Tree02/tree02.obj", &diffuse_textures);
            texture_path = "Resources/Models/Trees/Tree02/";
        }

        ADRIA_ASSERT(diffuse_textures.size() == trees.size());

		std::vector<TreeInstance> instance_data{};
		for (int32 i = 0; i < params.tree_count; ++i)
		{
			static const uint32 MAX_ITERATIONS = 5;
			XMFLOAT3 position{};
			XMFLOAT3 normal{};
			uint32 iteration = 0;
			do
			{
				if (iteration > MAX_ITERATIONS) break;
				position.x = random_x();
				position.z = random_z();
				position.y = TerrainComponent::terrain ? TerrainComponent::terrain->HeightAt(position.x, position.z) - 0.5f : -0.5f;
				normal = TerrainComponent::terrain ? TerrainComponent::terrain->NormalAt(position.x, position.z) : XMFLOAT3(0.0f, 1.0f, 0.0f);

				++iteration;
			} while (position.y > params.tree_height_end || position.y < params.tree_height_start || normal.y < params.tree_slope_start);

			if (iteration < MAX_ITERATIONS) instance_data.emplace_back(position, random_angle());
		}

        for (size_t i = 0; i < trees.size(); ++i)
        {
            auto tree = trees[i];
			auto& mesh_component = reg.get<Mesh>(tree);

			mesh_component.instance_buffer = std::make_shared<Buffer>(gfx, VertexBufferDesc(instance_data.size(), sizeof(TreeInstance)), instance_data.data());
			mesh_component.start_instance_location = 0;
			mesh_component.instance_count = (uint32)instance_data.size();
			
			Material material{};
			material.albedo_texture = texture_manager.LoadTexture(texture_path + diffuse_textures[i]);
			material.albedo_factor = 1.0f;
			material.shader = EShaderProgram::GBuffer_Foliage;
			reg.emplace<Material>(tree, material);
			reg.emplace<Foliage>(tree);

			Transform transform{};
			transform.starting_transform = XMMatrixScaling(size, size, size);
			transform.current_transform = transform.starting_transform;
			reg.emplace<Transform>(tree, transform);

			Visibility vis{};
			vis.skip_culling = true;
			reg.emplace<Visibility>(tree, vis);
        }

		return trees;
	}

    entity ModelImporter::LoadEmitter(EmitterParameters const& params)
	{
		Emitter emitter{};
		emitter.position = DirectX::XMFLOAT4(params.position[0], params.position[1], params.position[2], 1);
		emitter.velocity = DirectX::XMFLOAT4(params.velocity[0], params.velocity[1], params.velocity[2], 0);
		emitter.position_variance = DirectX::XMFLOAT4(params.position_variance[0], params.position_variance[1], params.position_variance[2], 1);
        emitter.velocity_variance = params.velocity_variance;
		emitter.number_to_emit = 0;
		emitter.particle_lifespan = params.lifespan;
		emitter.start_size = params.start_size;
		emitter.end_size = params.end_size;
		emitter.mass = params.mass;
		emitter.particles_per_second = params.particles_per_second;
        emitter.sort = params.sort;
        emitter.collisions_enabled = params.collisions;
		emitter.particle_texture = texture_manager.LoadTexture(params.texture_path);

		tecs::entity emitter_entity = reg.create();
		reg.add(emitter_entity, emitter);

        if (params.name.empty()) reg.emplace<Tag>(emitter_entity);
        else reg.emplace<Tag>(emitter_entity, params.name);

        return emitter_entity;
	}

    entity ModelImporter::LoadDecal(DecalParameters const& params)
	{
        Decal decal{};
        if(!params.albedo_texture_path.empty()) decal.albedo_decal_texture = texture_manager.LoadTexture(params.albedo_texture_path);
        if(!params.normal_texture_path.empty()) decal.normal_decal_texture = texture_manager.LoadTexture(params.normal_texture_path);

        XMVECTOR P = XMLoadFloat4(&params.position);
        XMVECTOR N = XMLoadFloat4(&params.normal);

        XMVECTOR ProjectorDirection = XMVectorNegate(N);
        XMMATRIX RotationMatrix = XMMatrixRotationAxis(ProjectorDirection, params.rotation);
        XMMATRIX model_matrix = XMMatrixScaling(params.size, params.size, params.size) * RotationMatrix * XMMatrixTranslationFromVector(P);

        decal.decal_model_matrix = model_matrix;
		decal.modify_gbuffer_normals = params.modify_gbuffer_normals;

		XMFLOAT3 abs_normal;
		XMStoreFloat3(&abs_normal, XMVectorAbs(N));
		if (abs_normal.x >= abs_normal.y && abs_normal.x >= abs_normal.z)
		{
			decal.decal_type = EDecalType::Project_YZ;
		}
		else if (abs_normal.y >= abs_normal.x && abs_normal.y >= abs_normal.z)
		{
			decal.decal_type = EDecalType::Project_XZ;
		}
		else
		{
			decal.decal_type = EDecalType::Project_XY;
		}
        
        entity decal_entity = reg.create();
        reg.add(decal_entity, decal);
		if (params.name.empty()) reg.emplace<Tag>(decal_entity, "decal");
		else reg.emplace<Tag>(decal_entity, params.name);

        return decal_entity;
	}

}