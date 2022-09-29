//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/loader/dae.h"

#include "core/dynamic_array.inl"
#include "core/fixed_array.inl"
#include "core/hash_table.inl"
#include "core/linear_allocator.inl"
#include "core/loader/xml.h"
#include "core/log.h"
#include "core/math/quat.inl"
#include "core/math/vec4.inl"
#include "core/string.inl"
#include "core/utils.h"

#include <ctype.h>
#include <math.h>

union Source_array_t_{
  Dynamic_array_t<float> float_array;
  Dynamic_array_t<Cstring_t> name_array;
};

M4_t parse_m4_(const char* s, const char** out) {
  M4_t rv;
  char** temp_out = (char**)&s;
  for (int i = 0; i < 16; ++i) {
    rv.a[i] = strtof(*temp_out, temp_out);
  }
  maybe_assign(out, *(const char**)temp_out);
  return rv;
}

void update_inv_bind_matrix_(Dynamic_array_t<M4_t>* inv_bind_matrices, Joint_t* joint) {
  for (auto& child : joint->children) {
    if ((*inv_bind_matrices)[child->mat_idx] == (M4_t){}) {
      (*inv_bind_matrices)[child->mat_idx] = (*inv_bind_matrices)[joint->mat_idx];
    }
    update_inv_bind_matrix_(inv_bind_matrices, child);
  }
}

void parse_geometry_node_(Linear_allocator_t<>* allocator,
                          Dynamic_array_t<Vertex_t>* m_vertices,
                          const Xml_node_t* geometry,
                          const Hash_map_t<Cstring_t, Source_array_t_>& sources,
                          const Dynamic_array_t<Cstring_t>* joints,
                          int mat_idx,
                          const Dynamic_array_t<float>* weights,
                          const Hash_map_t<Cstring_t, Joint_t*>* joint_map,
                          const Xml_node_t* vertex_weights,
                          const M4_t& bind_shape_matrix,
                          int stride,
                          int joint_offset,
                          int weight_offset) {
  Scope_allocator_t<> temp_allocator(allocator);
  auto position_semantic = geometry->find_first_by_tag("vertices")->find_first_by_attr("semantic", "POSITION");
  const Dynamic_array_t<float>& positions = sources.find(position_semantic->m_attributes.find("source")->get_substr(1))->float_array;

  int position_count = positions.len() / 3;
  Dynamic_array_t<Vertex_t> vertices(&temp_allocator);
  vertices.resize(position_count);
  if (vertex_weights) {
    int vertex_weights_count = atoi(vertex_weights->m_attributes.find("count")->m_p);
    M_check(positions.len() / 3 == vertex_weights_count);

    char* vcount_p = vertex_weights->find_first_by_tag("vcount")->m_text.m_p;
    char* v_p = vertex_weights->find_first_by_tag("v")->m_text.m_p;
    for (int i = 0; i < position_count; ++i) {
      // Only |position|, |joints_idx|, and |weights| are filled for now
      Vertex_t& vertex = vertices[i];
      vertex = Vertex_t();
      V3_t pos = ((V3_t*)positions.m_p)[i];
      vertex.position = (V4_t){pos.x, pos.y, pos.z, 1.f};
      vertex.position = bind_shape_matrix * vertex.position;
      int vcount = strtof(vcount_p, &vcount_p);
      float total_weight = 0.f;
      for (int j = 0; j < vcount; ++j) {
        for (int k = 0; k < stride; ++k) {
          int v = strtol(v_p, &v_p, 10);
          if (k == joint_offset) {
            M_check(v != -1);
            vertex.joint_idx[j] = (*joint_map->find((*joints)[v]))->mat_idx;
          } else if (k == weight_offset) {
            vertex.weights[j] = (*weights)[v];
            total_weight += (*weights)[v];
          }
        }
      }
      // Normalize weight
      for (int j = 0; j < vcount; ++j) {
        vertex.weights[j] /= total_weight;
      }
    }
  } else {
    for (int i = 0; i < position_count; ++i) {
      Vertex_t& vertex = vertices[i];
      vertex = Vertex_t();
      V3_t pos = ((V3_t*)positions.m_p)[i];
      vertex.position = (V4_t){pos.x, pos.y, pos.z, 1.f};
      vertex.joint_idx[0] = mat_idx;
      vertex.weights[0] = 1.f;
    }
  }

  Xml_nodes_t triangles_tags(&temp_allocator);
  geometry->find_all_by_tag(&triangles_tags, "triangles");
  int vertex_count = 0;
  for (auto triangles_tag : triangles_tags) {
    vertex_count += atoi(triangles_tag->m_attributes.find("count")->m_p);
  }
  m_vertices->reserve(m_vertices->len() + vertex_count);
  for (auto triangles_tag : triangles_tags) {
    int triangle_count = atoi(triangles_tag->m_attributes.find("count")->m_p);
    int triangle_stride = triangles_tag->count_all_by_tag("input");

    auto vertex_semantic = triangles_tag->find_first_by_attr("semantic", "VERTEX");
    int vertex_offset = atoi(vertex_semantic->m_attributes.find("offset")->m_p);

    auto normal_semantic = triangles_tag->find_first_by_attr("semantic", "NORMAL");
    int normal_offset = -1;
    const Dynamic_array_t<float>* normals = NULL;
    if (normal_semantic) {
      normal_offset = atoi(normal_semantic->m_attributes.find("offset")->m_p);
      normals = &sources.find(normal_semantic->m_attributes.find("source")->get_substr(1))->float_array;
    }

    char* triangle_p = triangles_tag->find_first_by_tag("p")->m_text.m_p;
    for (int i = 0; i < triangle_count; ++i) {
      for (int j = 0; j < 3; ++j) {
        Vertex_t vertex;
        for (int k = 0; k < triangle_stride; ++k) {
          int index = strtol(triangle_p, &triangle_p, 10);
          if (k == vertex_offset) {
            vertex.position = vertices[index].position;
            memcpy(&vertex.joint_idx, &vertices[index].joint_idx, sizeof(vertex.joint_idx));
            memcpy(&vertex.weights, &vertices[index].weights, sizeof(vertex.weights));
          } else if (k == normal_offset) {
            vertex.normal = ((V3_t*)normals->m_p)[index];
          }
        }
        m_vertices->append(vertex);
      }
    }
  }

  Xml_nodes_t polylist_tags(&temp_allocator);
  geometry->find_all_by_tag(&polylist_tags, "polylist");
  vertex_count = 0;
  for (auto polylist_tag : polylist_tags) {
    vertex_count += atoi(polylist_tag->m_attributes.find("count")->m_p);
  }
  m_vertices->reserve(m_vertices->len() + vertex_count);
  for (auto polylist_tag : polylist_tags) {
    int triangle_count = atoi(polylist_tag->m_attributes.find("count")->m_p);
    int triangle_stride = polylist_tag->count_all_by_tag("input");

    auto vertex_semantic = polylist_tag->find_first_by_attr("semantic", "VERTEX");
    int vertex_offset = atoi(vertex_semantic->m_attributes.find("offset")->m_p);

    auto normal_semantic = polylist_tag->find_first_by_attr("semantic", "NORMAL");
    int normal_offset = -1;
    const Dynamic_array_t<float>* normals = NULL;
    if (normal_semantic) {
      normal_offset = atoi(normal_semantic->m_attributes.find("offset")->m_p);
      normals = &sources.find(normal_semantic->m_attributes.find("source")->get_substr(1))->float_array;
    }

    char* p = polylist_tag->find_first_by_tag("p")->m_text.m_p;
    for (int i = 0; i < triangle_count; ++i) {
      for (int j = 0; j < 3; ++j) {
        Vertex_t vertex;
        for (int k = 0; k < triangle_stride; ++k) {
          int index = strtol(p, &p, 10);
          if (k == vertex_offset) {
            vertex.position = vertices[index].position;
            memcpy(&vertex.joint_idx, &vertices[index].joint_idx, sizeof(vertex.joint_idx));
            memcpy(&vertex.weights, &vertices[index].weights, sizeof(vertex.weights));
          } else if (k == normal_offset) {
            vertex.normal = ((V3_t*)normals->m_p)[index];
          }
        }
        m_vertices->append(vertex);
      }
    }
  }
}

void build_joint_hierarchy_(Dae_loader_t* loader,
                            const Xml_node_t* root,
                            Linear_allocator_t<>* temp_allocator,
                            const Hash_map_t<Cstring_t, Source_array_t_>& sources,
                            const Xml_node_t* node,
                            const M4_t& parent_mat,
                            Joint_t* joint,
                            Hash_map_t<Cstring_t, Joint_t*>* map,
                            Dynamic_array_t<M4_t>* matrices) {
  Cstring_t id = *node->m_attributes.find("id");
  const Xml_node_t* matrix = node->find_first_by_path("matrix");
  if (matrix) {
    joint->default_mat = parse_m4_(matrix->m_text.m_p, NULL);
  } else {
    joint->default_mat = m4_identity();
  }
  matrices->append(parent_mat * joint->default_mat);
  joint->mat_idx = matrices->len() - 1;
  (*map)[id] = joint;
  for (const auto& child : node->m_children) {
    if (child->m_tag_name == "instance_geometry") {
      const Xml_node_t* geometry = root->find_first_by_attr("id", child->m_attributes.find("url")->get_substr(1));
      parse_geometry_node_(temp_allocator, &loader->m_vertices, geometry, sources, NULL, joint->mat_idx, NULL, NULL, NULL, m4_identity(), -1, -1, -1);
    } else if (child->m_tag_name == "node") {
      // TODO: do we have to check that type == "JOINT"?
      Joint_t* child_joint = joint->children.m_allocator->construct<Joint_t>(joint->children.m_allocator);
      joint->children.append(child_joint);
      build_joint_hierarchy_(loader, root, temp_allocator, sources, child, (*matrices)[joint->mat_idx], child_joint, map, matrices);
    }
  }
}

void update_joint_hierarchy_(Dynamic_array_t<M4_t>* matrices, const Joint_t* parent) {
  for (auto child : parent->children) {
    M4_t& child_mat = (*matrices)[child->mat_idx];
    child_mat = (*matrices)[parent->mat_idx] * child_mat;
    update_joint_hierarchy_(matrices, child);
  }
}

void anim_destroy(Animation_t* animation) {
  animation->matrices.destroy();
  animation->times.destroy();
}

M4_t anim_get_at(const Animation_t* animation, float time) {
  float time_mod = fmod(time, animation->duration);
  for (int i = 0; i < animation->times.len() - 1; ++i) {
    if (time_mod >= animation->times[i] && time_mod < animation->times[i+1]) {
      const M4_t& m1 = animation->matrices[i];
      const M4_t& m2 = animation->matrices[i + 1];
      Quat_t q1 = quat_from_m4(m1);
      Quat_t q2 = quat_from_m4(m2);
      for (int j = 0; j < 4; ++j) {
        Quat_t sub;
        sub.v = q1.v - q2.v;
        Quat_t sum;
        sum.v = q1.v + q2.v;
        if (quat_norm(sub) > quat_norm(sum)) {
          q2.v = q2.v * -1.f;
          break;
        }
      }
      M4_t rv = quat_to_m4(quat_lerp(q1, q2, animation->times[i], animation->times[i+1], time_mod));
      V4_t translate1 = V4_t{m1.m[0][3], m1.m[1][3], m1.m[2][3], 1.f};
      V4_t translate2 = V4_t{m2.m[0][3], m2.m[1][3], m2.m[2][3], 1.f};
      V4_t lerped_translate = vec4_lerp(translate1, translate2, animation->times[i], animation->times[i+1], time_mod);
      rv.m[0][3] = lerped_translate.a[0];
      rv.m[1][3] = lerped_translate.a[1];
      rv.m[2][3] = lerped_translate.a[2];
      return rv;
    }
  }
  M_unimplemented();
  return M4_t();
}

Dae_loader_t::Dae_loader_t(Allocator_t* allocator)
    : m_vertices(allocator), m_animations(allocator), m_joint_matrices(allocator), m_inv_bind_matrices(allocator), m_root_joint(allocator) {}

bool Dae_loader_t::init(const Path_t& path) {
  Linear_allocator_t<> temp_allocator("temp_allocator");
  M_scope_exit(temp_allocator.destroy());
  Xml_t xml(&temp_allocator);
  xml.init(path);

  Xml_nodes_t source_nodes(&temp_allocator);
  xml.m_root->find_all_by_tag(&source_nodes, "source");
  Hash_map_t<Cstring_t, Source_array_t_> sources(&temp_allocator);
  sources.reserve(sources.len());
  for (auto source_node : source_nodes) {
    {
      auto float_array = source_node->find_first_by_tag("float_array");
      if (float_array) {
        Dynamic_array_t<float> floats(&temp_allocator);
        int count = atoi(float_array->m_attributes.find("count")->m_p);
        floats.resize(count);
        char* p = float_array->m_text.m_p;
        for (int i = 0; i < count; ++i) {
          float x = strtof(p, &p);
          floats[i] = x;
        }
        sources[*source_node->m_attributes.find("id")].float_array = floats;
      }
    }
    {
      auto name_array = source_node->find_first_by_tag("Name_array");
      if (name_array) {
        Dynamic_array_t<Cstring_t> names(&temp_allocator);
        int count = atoi(name_array->m_attributes.find("count")->m_p);
        names.resize(count);
        char* p = name_array->m_text.m_p;
        for (int i = 0; i < count; ++i) {
          while (isspace(*p)) {
            ++p;
          }
          char* start = p++;
          while (!isspace(*p) && *p != 0) {
            ++p;
          }
          names[i] = Cstring_t(start, p);
        };
        sources[*source_node->m_attributes.find("id")].name_array = names;
      }
    }
  }

  Hash_map_t<Cstring_t, Joint_t*> joint_map(&temp_allocator);
  const Xml_node_t* root_joint = xml.m_root->find_first_by_tag("visual_scene")->find_first_by_tag("node");
  build_joint_hierarchy_(this, xml.m_root, &temp_allocator, sources, root_joint, m4_identity(), &m_root_joint, &joint_map, &m_joint_matrices);

  Xml_nodes_t controllers(&temp_allocator);
  xml.m_root->find_first_by_path("library_controllers")->find_all_by_tag(&controllers, "controller");

  m_inv_bind_matrices.resize(m_joint_matrices.len());
  for (auto& m : m_inv_bind_matrices) {
    m = m4_identity();
  }
  // We only parse meshes referred by <skin>
  Dynamic_array_t<Dynamic_array_t<Vertex_t>> meshes(&temp_allocator);
  for (const auto& controller : controllers) {
    const Xml_node_t* skin = controller->find_first_by_tag("skin");
    M4_t bind_shape_matrix = m4_identity();
    {
      const Xml_node_t* bind_shape_matrix_tag = skin->find_first_by_tag("bind_shape_matrix");
      bind_shape_matrix = parse_m4_(bind_shape_matrix_tag->m_text.m_p, NULL);
    }
    auto vertex_weights = controller->find_first_by_tag("vertex_weights");
    auto joint_semantic = vertex_weights->find_first_by_attr("semantic", "JOINT");
    int joint_offset = atoi(joint_semantic->m_attributes.find("offset")->m_p);
    const Dynamic_array_t<Cstring_t>& joints = sources.find(joint_semantic->m_attributes.find("source")->get_substr(1))->name_array;

    auto inv_bind_matrix_semantic = controller->find_first_by_tag("joints")->find_first_by_attr("semantic", "INV_BIND_MATRIX");
    const Dynamic_array_t<float>& inv_bind_matrices = sources.find(inv_bind_matrix_semantic->m_attributes.find("source")->get_substr(1))->float_array;
    // TODO: INV_BIND_MATRIX can duplicate multiple times
    for (int i = 0; i < joints.len(); ++i) {
      int idx = (*joint_map.find(joints[i]))->mat_idx;
      m_inv_bind_matrices[idx] = ((M4_t*)inv_bind_matrices.m_p)[i];
    }

    int stride = vertex_weights->count_all_by_tag("input");

    auto weight_semantic = vertex_weights->find_first_by_attr("semantic", "WEIGHT");
    int weight_offset = atoi(weight_semantic->m_attributes.find("offset")->m_p);
    const Dynamic_array_t<float>& weights = sources.find(weight_semantic->m_attributes.find("source")->get_substr(1))->float_array;

    const Xml_node_t* geometry = xml.m_root->find_first_by_attr("id", skin->m_attributes.find("source")->get_substr(1));
    parse_geometry_node_(&temp_allocator,
                         &m_vertices,
                         geometry,
                         sources,
                         &joints,
                         -1,
                         &weights,
                         &joint_map,
                         vertex_weights,
                         bind_shape_matrix,
                         stride,
                         joint_offset,
                         weight_offset);
  }
  // update_inv_bind_matrix_(&m_inv_bind_matrices, &m_root_joint);

  {
    const Xml_node_t* animations_node = xml.m_root->find_first_by_path("library_animations");
    m_animations.reserve(animations_node->m_children.len());
    for (const auto& animation_node : animations_node->m_children) {
      // TODO: <animation> can contain <animation> as child
      Animation_t animation(m_vertices.m_allocator);
      const Xml_node_t* sampler = animation_node->find_first_by_tag("sampler");
      {
        Cstring_t animation_times_id = sampler->find_first_by_attr("semantic", "INPUT")->m_attributes.find("source")->get_substr(1);
        const Dynamic_array_t<float>& animation_times = sources.find(animation_times_id)->float_array;
        animation.times.resize(animation_times.len());
        memcpy(animation.times.m_p, animation_times.m_p, animation_times.len() * sizeof(float));
        animation.duration = animation.times.last();
      }
      {
        Cstring_t animation_matrices_id = sampler->find_first_by_attr("semantic", "OUTPUT")->m_attributes.find("source")->get_substr(1);
        const Dynamic_array_t<float>& animation_matrices = sources.find(animation_matrices_id)->float_array;
        M_check(animation_matrices.len() == animation.times.len() * 16);
        animation.matrices.resize(animation.times.len());
        memcpy(animation.matrices.m_p, animation_matrices.m_p, animation.times.len() * sizeof(M4_t));
      }
      auto channel_node = animation_node->find_first_by_tag("channel");
      M_check(channel_node->m_tag_name == "channel");
      Cstring_t target = channel_node->m_attributes.find("target")->to_const();
      Sip slash_index;
      M_check(target.find_char(&slash_index, '/'));
      Joint_t** joint = joint_map.find(target.get_substr(0, slash_index));
      if (joint) {
        animation.joint = *joint;
        m_animations.append(animation);
      }
    }
  }
  return true;
}

void Dae_loader_t::destroy() {
}

void Dae_loader_t::update_joint_matrices_at(float time_s) {
  for (int i = 0; i < m_animations.len(); ++i) {
    m_joint_matrices[m_animations[i].joint->mat_idx] = anim_get_at(&m_animations[i], time_s);
  }
  update_joint_hierarchy_(&m_joint_matrices, &m_root_joint);
}
