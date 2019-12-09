#include <iostream>

#include "debug_log.h"
#include "model.h"
#include "texture_manager.h"

using namespace DebugUtility;
using namespace std;

void Model::LoadFromFile(const string& filename) {
  ifstream fin(filename);
  if (!fin)
  {
    wstring box_msg = DebugUtility::AnsiToWString(filename);
    box_msg += L" not found.";
    MessageBox(0, box_msg.c_str(), 0, 0);
    return;
  }

  string ignore;
  string meshname;

  fin >> ignore >> meshname;

  mesh_ = MeshManager::Instance().LoadFromFile(meshname);

  fin >> ignore >> ignore >> ignore;

  fin >> material_.DiffuseAlbedo.x >> material_.DiffuseAlbedo.y 
      >> material_.DiffuseAlbedo.z >> material_.DiffuseAlbedo.w;

  fin >> ignore >> material_.FresnelR0.x >> material_.FresnelR0.y >> material_.FresnelR0.z;

  fin >> ignore >> material_.Roughness >> ignore;

  material_.MatTransform = MathHelper::Identity4x4();
  XMMATRIX matTransform = XMLoadFloat4x4(&material_.MatTransform);
  XMStoreFloat4x4(&material_.MatTransform, XMMatrixTranspose(matTransform));

  int texture_count;

  fin >> ignore >> texture_count >> ignore;
  SRVs.resize(texture_count);
  string texture_name;

  for (int i=0; i< texture_count; ++i){
    fin >> ignore >>  texture_name;
    wstring texture_name_w = DebugUtility::AnsiToWString(texture_name);
    auto* texture = TextureManager::Instance().RequestTexture(texture_name_w);
    SRVs[i] = texture->SrvHandle();
  }
  fin >> ignore;
  fin.close();

  name = filename;
}

void Mesh::LoadMesh(const string& filename) {
  ifstream fin(filename);

  UINT vertex_count = 0;
  UINT index_count = 0;
  std::string ignore;

  if (!fin)
  {
    wstring box_msg = DebugUtility::AnsiToWString(filename);
    box_msg += L" not found.";
    MessageBox(0, box_msg.c_str(), 0, 0);
    return;
  }

  fin >> ignore >> vertex_count;
  fin >> ignore >> index_count;
  fin >> ignore >> ignore >> ignore >> ignore;

  vector<Vertex> vertices(vertex_count);
  vector<uint16_t> indices(index_count * 3);

  XMFLOAT3 v_min_f3 = XMFLOAT3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
  XMFLOAT3 v_max_f3 = XMFLOAT3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

  XMVECTOR v_min = XMLoadFloat3(&v_min_f3);
  XMVECTOR v_max = XMLoadFloat3(&v_max_f3);

  //  DirectX::XMFLOAT3 ignore_normal;

  for (UINT i = 0; i < vertex_count; ++i) {
    fin >> vertices[i].Position.x >> vertices[i].Position.y >> vertices[i].Position.z;
    fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
    vertices[i].TexCoord = { 0.0f, 0.0f };
    XMVECTOR pos = XMLoadFloat3(&vertices[i].Position);

    XMFLOAT3 spherePos;
    XMStoreFloat3(&spherePos, XMVector3Normalize(pos));

    float theta = atan2f(spherePos.z, spherePos.x);

    // Put in [0, 2pi].
    if (theta < 0.0f)
      theta += XM_2PI;

    float phi = acosf(spherePos.y);

    float u = theta / (2.0f * XM_PI);
    float v = phi / XM_PI;

    //  vertices_geo[i].TexCoord = { 0.0f, 0.0f };
    vertices[i].TexCoord = { u, v };

    v_min = XMVectorMin(v_min, pos);  //  find the min point
    v_max = XMVectorMax(v_max, pos);  //  find the max point
  }

  BoundingBox bounds;
  //XMStoreFloat3(&bounds.Center, 0.5f * (v_min + v_max));
  //XMStoreFloat3(&bounds.Extents, 0.5f * (v_max - v_min));
  
  //BoundingSphere bounds;
  
  //XMStoreFloat3(&bounds.Center, 0.5f * (v_min + v_max));
  //XMFLOAT3 d;
  //XMStoreFloat3(&d, 0.5f * (v_max - v_min ));
  //float ratio = 0.5f;
  //bounds.Radius = ratio * sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
  
  
  fin >> ignore >> ignore >> ignore;
  for (UINT i = 0; i < index_count; ++i) {
    fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
  }
  
  fin.close();
  vertex_buffer_.Create(L"Vertex Buffer",
    (uint32_t)vertices.size(), sizeof(Vertex), vertices.data());
  index_buffer_.Create(L"Index Buffer",
    (uint32_t)indices.size(), sizeof(uint16_t), indices.data());
}

Mesh* MeshManager::LoadFromFile(const string& filename) {
  map<string, Mesh*>::iterator it = mesh_pool_.find(filename);
  if (it != mesh_pool_.end()) {
    return it->second;
  }

  Mesh* mesh = new Mesh();
  mesh->LoadMesh(filename);
  mesh_pool_.emplace(filename, mesh);

  return mesh_pool_[filename];
}

void Mesh::LoadFromObj(const string& filename) {
  ifstream fin(filename);
  if (!fin)
  {
    wstring box_msg = DebugUtility::AnsiToWString(filename);
    box_msg += L" not found.";
    MessageBox(0, box_msg.c_str(), 0, 0);
    return;
  }

  string value_line;
  string header;
  string ignore;
  istringstream iss_line;
  int vt_index=0;
  vector<Vertex> vertices;
  istringstream iss_face;
  string index_face;
  uint16_t index ;
  vector<uint16_t> indices;
  vector<uint16_t> temp_indices;
  vector<uint16_t> temp_indices_uv;
  vector<uint16_t> temp_indices_normal;
  vector<XMFLOAT2> texcoords;
  vector<XMFLOAT3> normals;
  XMFLOAT2 temp_texcoord;
  XMFLOAT3 temp_normal;


  while (std::getline(fin, value_line)) {
    //  iss.getline(const_cast<char*>(value.c_str()), value.size());
    iss_line.str(value_line);
    iss_line >> header;

    if (header == "v") {
      Vertex vertex;
      iss_line >> vertex.Position.x >> vertex.Position.y >> vertex.Position.z;
      vertices.push_back(vertex);
    }
    else if (header == "vt") {
      iss_line >> temp_texcoord.x >> temp_texcoord.y;
      texcoords.push_back(temp_texcoord);
    }
    else if (header == "vn") {
      iss_line >> temp_normal.x >> temp_normal.y >> temp_normal.z;
      normals.push_back(temp_normal);
    }
    else if (header == "f") {
      while (iss_line >> index_face) {
        if (index_face.size() > 0) {
          auto result = split(index_face, '/');
          iss_face.str(result[0]);
          iss_face >> index;
          temp_indices.push_back(index-1);
          iss_face.clear();

          if (result[1].size() > 0) { //  uv index
            iss_face.str(result[1]);
            iss_face >> index;
            temp_indices_uv.push_back(index-1);
            iss_face.clear();
          }

          if (result.size() > 2 && result[2].size() > 0) {  // normal index
            iss_face.str(result[2]);
            iss_face >> index;
            temp_indices_normal.push_back(index - 1);
            iss_face.clear();
          }
        }
      }

      indices.push_back(temp_indices[0]);
      indices.push_back(temp_indices[1]);
      indices.push_back(temp_indices[2]);

      if (temp_indices.size() == 4) {
        indices.push_back(temp_indices[0]);
        indices.push_back(temp_indices[2]);
        indices.push_back(temp_indices[3]);
      }

      for (int i = 0; i < temp_indices.size(); ++i) {
        uint16_t pos_idx = temp_indices[i];
        if (vertices.size() > pos_idx) {
          Vertex& vertex = vertices[pos_idx];
          if (temp_indices_uv.size() > 0) {
            auto texcoord = texcoords[temp_indices_uv[i]];
            vertex.TexCoord.x = texcoord.x;
            vertex.TexCoord.y = texcoord.y;
          }
          else {
            vertex.TexCoord.x = 0.0f;
            vertex.TexCoord.y = 0.0f;
          }

          if (temp_indices_normal.size() > 0) {
            auto normal = normals[temp_indices_normal[i]];
            vertex.Normal.x = normal.x;
            vertex.Normal.y = normal.y;
            vertex.Normal.z = normal.z;
          }
          else {
            vertex.Normal.x = 0.0f;
            vertex.Normal.y = 0.0f;
            vertex.Normal.z = 0.0f;
          }
        }
        else {

          cout << "error, vertices size = " << vertices.size() << pos_idx << endl;
        }
      }

      temp_indices.clear();
      temp_indices_uv.clear();
    }

    header.clear();
    iss_line.clear();
  }

  fin.close();
  vertex_buffer_.Create(L"Vertex Buffer",
    (uint32_t)vertices.size(), sizeof(Vertex), vertices.data());
  index_buffer_.Create(L"Index Buffer",
    (uint32_t)indices.size(), sizeof(uint16_t), indices.data());
}