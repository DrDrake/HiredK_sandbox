/**
* @file Model.h
* @brief
*/

#pragma once

#include "FileHandle.h"
#include "Camera.h"
#include "Texture.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "Math.h"

namespace tinygltf
{
	class Scene;
}

class btCompoundShape;

namespace root
{
	class Model : public FileHandle, public Transform
	{
		REGISTER_FILE(Model)

		public:
			//! CTOR/DTOR:
			Model(Engine* engine_instance, const std::string& path);
			virtual ~Model();

			//! SERVICES:
			void Draw(Camera& camera);
			void Draw();
			std::shared_ptr<Transform> GetNodeTransform(const std::string& name);
			btCompoundShape* BuildCollider() const;

			//! VIRTUALS:
			virtual bool IsProcessBackground();

		protected:
			//! VIRTUALS:
			virtual bool CreateData();
			virtual void DeleteData();

			std::map<std::string, GLuint> m_buffers;
			std::map<std::string, std::shared_ptr<Material>> m_materials;
			tinygltf::Scene* m_scene;
	};

	////////////////////////////////////////////////////////////////////////////////
	// Model inline implementation:
	////////////////////////////////////////////////////////////////////////////////
	/*----------------------------------------------------------------------------*/
	inline Model::Model(Engine* engine_instance, const std::string& path)
		: FileHandle(engine_instance, path) {
	}
	/*----------------------------------------------------------------------------*/
	inline Model::~Model() {
	}

	/*----------------------------------------------------------------------------*/
	inline void Model::Draw(Camera& camera) {
		if (camera.GetFrustum().IsInside(GetBoundingBox().Transform(ComputeMatrix())) != Frustum::Invisible) {
			Shader::Get()->UniformMat4("u_ViewProjMatrix", camera.GetViewProjMatrix());
			Draw();
		}
	}

	/*----------------------------------------------------------------------------*/
	inline bool Model::IsProcessBackground() {
		return false;
	}

} // root namespace