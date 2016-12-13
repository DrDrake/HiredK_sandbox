#include "Model.h"
#include "Engine.h"
#include "FileSystem.h"
#include "Logger.h"

#define TINYGLTF_LOADER_IMPLEMENTATION
#include <btBulletDynamicsCommon.h>
#include <tiny_gltf_loader.h>

REGISTER_FILE_IMPL(root::Model, 1, "glb");

bool root::Model::CreateData()
{
	if (!m_scene) {
		if (!FileHandle::CreateData()) {
			return false;
		}

		m_scene = new tinygltf::Scene();
		GetBoundingBox().Reset();

		tinygltf::TinyGLTFLoader loader;
		std::string err;

		Logger::ScopedTimer timer(LOG, "Model::CreateData()");
		const unsigned char* bytes = static_cast<const unsigned char*>((const void*)&m_data[0]);
		bool ret = loader.LoadBinaryFromMemory(m_scene, &err, bytes, static_cast<unsigned int>(GetSize()));
		FileHandle::DeleteData();

		if (!err.empty()) {
			LOG(Logger::Error) << err;
		}

		// 1) LOAD SCENE BUFFERS
		for (auto const& it : m_scene->bufferViews) {
			const tinygltf::BufferView& bufferView = it.second;
			if (bufferView.target == 0) {
				// LOG(Logger::Warning) << "bufferView.target is zero"; // Useful message, but pollutes output
				continue;
			}

			const tinygltf::Buffer& buffer = m_scene->buffers[bufferView.buffer];

			GLuint state;
			glGenBuffers(1, &state);
			glBindBuffer(bufferView.target, state);
			glBufferData(bufferView.target, bufferView.byteLength, &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
			glBindBuffer(bufferView.target, GL_ZERO);
			m_buffers[it.first] = state;
		}

		// (Debug) DISPLAY NODE NAMES
		for (auto const& it : m_scene->nodes) {
			LOG() << "Adding '" << it.second.name << "' node";
		}

		// 2) PARSE SCENE MESHES
		for (auto const& it : m_scene->meshes) {
			const tinygltf::Mesh& mesh = it.second;

			for (size_t id = 0; id < mesh.primitives.size(); ++id) {
				const tinygltf::Primitive& primitive = mesh.primitives[id];
				if (primitive.material.empty()) {
					continue;
				}

				// 3) ENLARGE GLOBAL BOUNDING BOX
				for (auto const& it : primitive.attributes) {
					const tinygltf::Accessor& accessor = m_scene->accessors[it.second];
					if (it.first.compare("POSITION") == 0) {
						GetBoundingBox().Enlarge(dvec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]));
						GetBoundingBox().Enlarge(dvec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]));
					}
				}

				// 4) PARSE PRIMITIVE MATERIAL
				tinygltf::Material& material = m_scene->materials[primitive.material];
				if (m_materials.find(material.name) == m_materials.end())
				{
					std::shared_ptr<Material> new_material = std::shared_ptr<Material>(new Material(material.name));
					m_materials[material.name] = new_material;

					auto LoadMaterialMap = [this, &material, &new_material](const std::string& type) {
						if (material.values.find(type) != material.values.end()) {
							const std::string path = material.values[type].string_value;
							tinygltf::Texture& tex = m_scene->textures[path];

							if (m_scene->images.find(tex.source) != m_scene->images.end()) {
								tinygltf::Image& image = m_scene->images[tex.source];

								GLenum format = GL_RGBA;
								if (image.component == 3) {
									format = GL_RGB;
								}

								Texture::Settings& settings = Texture::GetDefaultSettings();
								settings.target = tex.target;
								settings.iformat = tex.internalFormat;
								settings.format = format;
								settings.type = tex.type;
								settings.filter_mode = GL_LINEAR_MIPMAP_LINEAR;
								settings.wrap_mode = GL_REPEAT;
								settings.anisotropy = 16.0f;
								settings.mipmap = true;

								if (type == "diffuse") {
									new_material->SetAlbedoMap(Texture::CreateTexture(image.width, image.height, 0, 0, &image.image.at(0), settings));
								}
								if (type == "bump") {
									new_material->SetNormalMap(Texture::CreateTexture(image.width, image.height, 0, 0, &image.image.at(0), settings));
								}
							}
						}
					};

					LoadMaterialMap("diffuse");
					LoadMaterialMap("bump");
				}
			}
		}
	}

	return true;
}

void root::Model::Draw()
{
	std::function<void(const tinygltf::Node& node)> DrawNode;
	DrawNode = [this, &DrawNode](const tinygltf::Node& node)
	{
		Shader::Get()->UniformMat4("u_ModelMatrix", Transform::ComputeMatrix() * glm::make_mat4(&node.matrix[0]));
		Shader::Get()->UniformMat3("u_NormalMatrix", mat3(glm::mat4_cast(m_rotation)));

		// 2) TRAVERSE NODE MESHES
		for (auto const& it : node.meshes) {
			const tinygltf::Mesh& mesh = m_scene->meshes[it];

			// 3) TRAVERSE NODE PRIMITIVES
			for (size_t id = 0; id < mesh.primitives.size(); ++id) {
				const tinygltf::Primitive& primitive = mesh.primitives[id];
				if (primitive.material.empty()) {
					continue;
				}

				// 4) BIND NODE PRIMITIVE MATERIAL
				tinygltf::Material& material = m_scene->materials[primitive.material];
				if (m_materials[material.name]) {
					m_materials[material.name]->Bind();
				}

				// 5) BIND NODE PRIMITIVE ATTRIBUTES
				for (auto const& it : primitive.attributes) {
					const tinygltf::Accessor& accessor = m_scene->accessors[it.second];
					glBindBuffer(GL_ARRAY_BUFFER, m_buffers[accessor.bufferView]);

					int count = 1;
					switch (accessor.type) {
						case TINYGLTF_TYPE_SCALAR:
							count = 1;
							break;
						case TINYGLTF_TYPE_VEC2:
							count = 2;
							break;
						case TINYGLTF_TYPE_VEC3:
							count = 3;
							break;
						case TINYGLTF_TYPE_VEC4:
							count = 4;
							break;
					}

					auto BindAttribute = [&it, &accessor, &count](const std::string& name, int index) {
						if (it.first.compare(name) == 0) {
							glVertexAttribPointer(index, count, accessor.componentType, GL_FALSE, static_cast<int>(accessor.byteStride), BUFFER_OFFSET(accessor.byteOffset));
							glEnableVertexAttribArray(index);
						}
					};

					// Replicate Mesh::Vertex3D 
					BindAttribute("POSITION", 0);
					BindAttribute("TEXCOORD_0", 1);
					BindAttribute("COLOR_0", 2);
					BindAttribute("NORMAL", 3);
					BindAttribute("TEXTANGENT", 4);
				}

				int mode = -1;
				switch (primitive.mode) {
					case TINYGLTF_MODE_TRIANGLES:
						mode = GL_TRIANGLES;
						break;
					case TINYGLTF_MODE_TRIANGLE_STRIP:
						mode = GL_TRIANGLE_STRIP;
						break;
					case TINYGLTF_MODE_TRIANGLE_FAN:
						mode = GL_TRIANGLE_FAN;
						break;
					case TINYGLTF_MODE_POINTS:
						mode = GL_POINTS;
						break;
					case TINYGLTF_MODE_LINE:
						mode = GL_LINES;
						break;
					case TINYGLTF_MODE_LINE_LOOP:
						mode = GL_LINE_LOOP;
						break;
				}

				// 6) BIND PRIMITIVE INDEX BUFFER
				const tinygltf::Accessor& indexAccessor = m_scene->accessors[primitive.indices];
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[indexAccessor.bufferView]);

				// 7) ACTUAL RENDERING CALL HERE!
				glDrawElements(mode, static_cast<int>(indexAccessor.count), indexAccessor.componentType, BUFFER_OFFSET(indexAccessor.byteOffset));
				for (int i = 0; i < primitive.attributes.size(); ++i) {
					glDisableVertexAttribArray(i);
				}
			}
		}

		for (auto const& it : node.children) {
			DrawNode(m_scene->nodes[it]);
		}
	};

	// 1) BEGIN RECURSIVE TRAVERSAL
	for (auto const& it : m_scene->nodes) {
		DrawNode(it.second);
	}
}

std::shared_ptr<root::Transform> root::Model::GetNodeTransform(const std::string& name)
{
	std::shared_ptr<Transform> transform = std::shared_ptr<Transform>(new Transform());
	bool node_found = false;

	for (auto& it : m_scene->nodes) {
		if (it.second.name != name) {
			continue;
		}

		dvec3 scale;
		dquat rotation;
		dvec3 position;
		dvec3 skew;
		dvec4 perspective;
		glm::decompose(glm::make_mat4(&(it.second.matrix[0])), scale, rotation, position, skew, perspective);

		for (auto const& it : it.second.meshes) {
			const tinygltf::Mesh& mesh = m_scene->meshes[it];

			for (size_t id = 0; id < mesh.primitives.size(); ++id) {
				const tinygltf::Primitive& primitive = mesh.primitives[id];

				auto it = primitive.attributes.find("POSITION");
				if (it == primitive.attributes.end() || primitive.material.empty()) {
					continue;
				}

				const tinygltf::Accessor& accessor = m_scene->accessors[(*it).second];
				transform->GetBoundingBox().Set(
					dvec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]),
					dvec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2])
				);
			}
		}

		transform->SetPosition(position);
		transform->SetRotation(glm::conjugate(rotation));
		node_found = true;
		break;
	}

	if (!node_found) {
		LOG(Logger::Warning) << "Model::GetNodeTransform() '" << name << "' node not found!";
	}

	return transform;
}

btCompoundShape* root::Model::BuildCollider() const
{
	btCompoundShape* compound = new btCompoundShape();

	std::function<void(const tinygltf::Node& node)> AddNodeCollider;
	AddNodeCollider = [this, &AddNodeCollider, &compound](const tinygltf::Node& node)
	{
		for (auto const& it : node.meshes) {
			const tinygltf::Mesh& mesh = m_scene->meshes[it];

			for (size_t id = 0; id < mesh.primitives.size(); ++id) {
				const tinygltf::Primitive& primitive = mesh.primitives[id];

				auto it = primitive.attributes.find("POSITION");
				if (it == primitive.attributes.end() || primitive.material.empty()) {
					continue;
				}

				const tinygltf::Accessor& vertices_accessor = m_scene->accessors[(*it).second];
				const tinygltf::Buffer& vertices_buffer = m_scene->buffers[m_scene->bufferViews[vertices_accessor.bufferView].buffer];
				const tinygltf::Accessor& indices_accessor = m_scene->accessors[primitive.indices];
				const tinygltf::Buffer& indices_buffer = m_scene->buffers[m_scene->bufferViews[indices_accessor.bufferView].buffer];

				if (vertices_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT &&
					indices_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					btTriangleIndexVertexArray* tiva = new btTriangleIndexVertexArray();
					btIndexedMesh indexed = btIndexedMesh();

					indexed.m_vertexBase = static_cast<const unsigned char*>(&vertices_buffer.data.at(0) + vertices_accessor.byteOffset);
					indexed.m_numVertices = vertices_accessor.count;
					indexed.m_vertexStride = 3 * sizeof(float);
					indexed.m_vertexType = PHY_FLOAT;

					indexed.m_triangleIndexBase = static_cast<const unsigned char*>(&indices_buffer.data.at(0) + m_scene->bufferViews[indices_accessor.bufferView].byteOffset);
					indexed.m_numTriangles = indices_accessor.count / 3;
					indexed.m_triangleIndexStride = 3 * sizeof(unsigned short);
					indexed.m_indexType = PHY_SHORT;

					tiva->addIndexedMesh(indexed, PHY_SHORT);

					btTransform transform;
					transform.setFromOpenGLMatrix(glm::value_ptr(glm::make_mat4(&node.matrix[0])));
					compound->addChildShape(transform, new btBvhTriangleMeshShape(tiva, true));
				}
			}
		}

		for (auto const& it : node.children) {
			AddNodeCollider(m_scene->nodes[it]);
		}
	};

	Logger::ScopedTimer timer(LOG, "Model::BuildCollider()");
	for (auto const& it : m_scene->nodes) {
		AddNodeCollider(it.second);
	}

	return compound;
}

void root::Model::DeleteData()
{
	if (m_scene) {
		m_buffers.clear();
		m_materials.clear();
		delete m_scene;
		m_scene = nullptr;
	}
}