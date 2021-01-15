#pragma once

// other project libs
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

// your project libs
#include "engine/opengl/mesh.hpp"
#include "engine/opengl/shader.hpp"
#include "engine/opengl/texture.hpp"
#include "engine/opengl/vertex.hpp"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

// https://github.com/ntcaston/AngryGL/tree/master/angrygl
// https://github.com/ntcaston/AngryGL/blob/master/angrygl/model.h
// https://github.com/ntcaston/AngryGL/blob/master/angrygl/player_mesh.h
// https://github.com/ntcaston/AngryGL/blob/master/angrygl/player_model.h

namespace fightingengine {

class Model
{
public:
  Model(const std::string& full_path);

  void draw(Shader& shader);

private:
  std::vector<Mesh> meshes;
  std::string directory;
  std::vector<Texture2D> loaded_textures;

private:
  void load_model(const std::string& path);
  void process_node(aiNode* node, const aiScene* scene);
  Mesh process_mesh(aiMesh* mesh, const aiScene* scene);
  std::vector<Texture2D> load_material_textures(aiMaterial* mat, aiTextureType type, TextureType type2);
};

} // namespace fightingengine