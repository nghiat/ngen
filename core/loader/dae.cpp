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

#include <math.h>

M4_t parse_m4_(const char* s, const char** out) {
  M4_t rv;
  char** temp_out = (char**)&s;
  for (int i = 0; i < 16; ++i) {
    rv.a[i] = strtof(*temp_out, temp_out);
  }
  maybe_assign(out, *(const char**)temp_out);
  return rv;
}

void build_joint_hierarchy_(const Xml_node_t* node, const M4_t& parent_mat, Joint_t* joint, Hash_map_t<Cstring_t, Joint_t*>* map) {
  Cstring_t id = *node->m_attributes.find("id");
  const Xml_node_t* matrix = node->find_child("matrix");
  joint->default_mat = parse_m4_(matrix->m_text.m_p, NULL);
  joint->mat = parent_mat * joint->default_mat;
  (*map)[id] = joint;
  for (const auto& child : node->m_children) {
    if (child->m_tag_name == "node") {
      Joint_t* child_joint = joint->children.m_allocator->construct<Joint_t>(joint->children.m_allocator);
      joint->children.append(child_joint);
      build_joint_hierarchy_(child, joint->mat, child_joint, map);
    }
  }
}

void update_joint_hierarchy_(const Joint_t* parent) {
  for (auto child : parent->children) {
    child->mat = parent->mat * child->mat;
    update_joint_hierarchy_(child);
  }
}

void anim_destroy(Animation_t* animation) {
  animation->matricies.destroy();
  animation->times.destroy();
}

M4_t anim_get_at(const Animation_t* animation, float time) {
  float time_mod = fmod(time, animation->duration);
  for (int i = 0; i < animation->times.len() - 1; ++i) {
    if (time_mod >= animation->times[i] && time_mod < animation->times[i+1]) {
      const M4_t& m1 = animation->matricies[i];
      const M4_t& m2 = animation->matricies[i + 1];
      Quat_t q1 = quat_from_m4(m1);
      Quat_t q2 = quat_from_m4(m2);
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

bool Dae_loader_t::init(Allocator_t* allocator, const Path_t& path) {
  m_vertices.m_allocator = allocator;
  m_index_buffer.m_allocator = allocator;
  m_inv_bind_matrices.m_allocator = allocator;
  m_animations.m_allocator = allocator;

  Linear_allocator_t<> xml_allocator("xml_allocator");
  M_scope_exit(xml_allocator.destroy());
  Xml_t xml(&xml_allocator);
  xml.init(path);
  const Xml_node_t* mesh_position = xml.m_root->find_child("library_geometries/geometry/mesh/source/float_array");
  int arr_len = atoi(mesh_position->m_attributes.find("count")->m_p);
  M_check_log_return_val(arr_len % 3 == 0, false, "Invalid vertex number");
  m_vertices.reserve(arr_len / 3);
  char* arr_p = mesh_position->m_text.m_p;
  while (*arr_p) {
    Vertex_t v = {};
    float x = strtof(arr_p, &arr_p);
    float y = strtof(arr_p, &arr_p);
    float z = strtof(arr_p, &arr_p);
    v.pos = {x, y, z, 1.0f};
    m_vertices.append(v);
  }
  M_check_log_return_val(arr_len / 3 == m_vertices.len(), false, "Unmatched vertex number between attribute and text");
  const Xml_node_t* mesh = xml.m_root->find_child("library_geometries/geometry/mesh");
  for (auto child : mesh->m_children) {
    if (child->m_tag_name.equals("polylist")) {
      int count = atoi(child->m_attributes.find("count")->m_p) * 3;
      m_index_buffer.reserve(m_index_buffer.len() + count);
      const Xml_node_t* p_node = child->find_child("p");
      M_check(p_node);
      char* index_p = p_node->m_text.m_p;
      for (int i = 0; i < count; ++i) {
        int v = strtol(index_p, &index_p, 10);
        m_index_buffer.append(v);
        // normal
        strtol(index_p, &index_p, 10);
        // texcoord
        strtol(index_p, &index_p, 10);
      }
    }
  }
  const Xml_node_t* bind_shape_matrix = xml.m_root->find_child("library_controllers/controller/skin/bind_shape_matrix");
  m_transform_matrix = parse_m4_(bind_shape_matrix->m_text.m_p, NULL);

  Hash_map_t<Cstring_t, Joint_t*> joint_map(&xml_allocator);
  const Xml_node_t* root_joint = xml.m_root->find_by_attr("id", "root");
  build_joint_hierarchy_(root_joint, m4_identity(), &m_root_joint, &joint_map);
  {
    {
      const Xml_node_t* joints_node = xml.m_root->find_child("library_controllers/controller/skin/joints");
      for (const auto& child : joints_node->m_children) {
        Cstring_t semantic = *child->m_attributes.find("semantic");
        if (semantic == "INV_BIND_MATRIX") {
          const Xml_node_t* inv_bind_matrix_source = xml.m_root->find_by_attr("id", child->m_attributes.find("source")->get_substr(1));
          const Xml_node_t* inv_bind_matrix = inv_bind_matrix_source->find_child("float_array");
          int inv_bind_matrix_count = atoi(inv_bind_matrix->m_attributes.find("count")->m_p);
          const char* text_p = inv_bind_matrix->m_text.m_p;
          for (int i = 0; i < inv_bind_matrix_count / 16; ++i) {
            m_inv_bind_matrices.append(parse_m4_(text_p, &text_p));
          }
        }
      }
    }
    const Xml_node_t* vertex_weights_node = xml.m_root->find_child("library_controllers/controller/skin/vertex_weights");
    Dynamic_array_t<float> weights(&xml_allocator);
    {
      Cstring_t weights_node_id = vertex_weights_node->find_by_attr("semantic", "WEIGHT")->m_attributes.find("source")->get_substr(1);
      const Xml_node_t* weights_node = xml.m_root->find_by_attr("id", weights_node_id)->find_first_tag("float_array");
      int weights_node_len = atoi(weights_node->m_attributes.find("count")->m_p);
      char* text_p = weights_node->m_text.m_p;
      weights.reserve(weights_node_len);
      for (int i = 0; i < weights_node_len; ++i) {
        weights.append(strtof(text_p, &text_p));
      }
    }
    {
      Cstring_t joints_node_id = vertex_weights_node->find_by_attr("semantic", "JOINT")->m_attributes.find("source")->get_substr(1);
      const Xml_node_t* joints_node = xml.m_root->find_by_attr("id", joints_node_id)->find_first_tag("Name_array");
      Cstring_t joints_text = joints_node->m_text.to_const();
      m_joint_refs.m_allocator = allocator;
      m_joint_refs.reserve(joint_map.len());
      Sip space_index;
      Sip from = 0;
      while(true) {
        Cstring_t joint_name;
        bool should_break = false;
        if (joints_text.find_char(&space_index, ' ', from)) {
          joint_name = joints_text.get_substr(from, space_index);
          from = space_index + 1;
        } else {
          joint_name = joints_text.get_substr(from);
          should_break = true;
        }
        m_joint_refs.append(&(*joint_map.find(joint_name))->mat);
        if (should_break) {
          break;
        }
      };
    }
    M_check(strtol(vertex_weights_node->m_attributes.find("count")->m_p, NULL, 10) == m_vertices.len());
    char* vcount_p = vertex_weights_node->find_first_tag("vcount")->m_text.m_p;
    char* v_p = vertex_weights_node->find_first_tag("v")->m_text.m_p;
    for (int i = 0; i < m_vertices.len(); ++i) {
      int v_count_val = strtol(vcount_p, &vcount_p, 10);
      float weight_sum = 0.0f;
      for (int j = 0; j < v_count_val; ++j) {
        int joint_idx = strtol(v_p, &v_p, 10);
        int weight_idx = strtol(v_p, &v_p, 10);
        if (joint_idx == -1) {
          m_vertices[i].joint_idx[j] = m_joint_refs.len();
        } else {
          m_vertices[i].joint_idx[j] = joint_idx;
        }
        m_vertices[i].weights[j] = weights[weight_idx];
        weight_sum += weights[weight_idx];
      }
      // Normalize
      for (int j = 0; j < v_count_val; ++j) {
        m_vertices[i].weights[j] /= weight_sum;
      }
    }
  }

  {
    const Xml_node_t* animations_node = xml.m_root->find_child("library_animations");
    m_animations.reserve(animations_node->m_children.len());
    for (const auto& animation_node : animations_node->m_children) {
      Animation_t animation(allocator);
      const Xml_node_t* sampler = animation_node->find_child("sampler");
      {
        Cstring_t animation_times_id = sampler->find_by_attr("semantic", "INPUT")->m_attributes.find("source")->get_substr(1);
        const Xml_node_t* animation_times = xml.m_root->find_by_attr("id", animation_times_id)->find_first_tag("float_array");
        int count = atoi(animation_times->m_attributes.find("count")->m_p);
        animation.times.resize(count);
        char* p = animation_times->m_text.m_p;
        for (int i = 0; i < count; ++i) {
          animation.times[i] = strtof(p, &p);
        }
        animation.duration = animation.times.last();
      }
      {
        Cstring_t animation_matrices_id = sampler->find_by_attr("semantic", "OUTPUT")->m_attributes.find("source")->get_substr(1);
        const Xml_node_t* animation_matrices = xml.m_root->find_by_attr("id", animation_matrices_id)->find_first_tag("float_array");
        int count = atoi(animation_matrices->m_attributes.find("count")->m_p);
        M_check(count == animation.times.len() * 16);
        count = animation.times.len();
        const char* p = animation_matrices->m_text.m_p;
        animation.matricies.resize(count);
        for (int i = 0; i < count; ++i) {
          animation.matricies[i] = parse_m4_(p, &p);
        }
      }
      const Xml_node_t* channel_node = animation_node->m_children.last();
      M_check(channel_node->m_tag_name == "channel");
      Cstring_t target = channel_node->m_attributes.find("target")->to_const();
      Sip slash_index;
      M_check(target.find_char(&slash_index, '/'));
      animation.joint = *joint_map.find(target.get_substr(0, slash_index));
      m_animations.append(animation);
    }
  }
  return true;
}

void Dae_loader_t::destroy() {
}

Dynamic_array_t<M4_t> Dae_loader_t::get_joint_matricies_at(Allocator_t* allocator, float time_s) {
  for (const auto& animation : m_animations) {
    animation.joint->mat = anim_get_at(&animation, time_s);
  }
  update_joint_hierarchy_(&m_root_joint);

  Dynamic_array_t<M4_t> rv(allocator);
  rv.resize(m_joint_refs.len() + 1);
  for (int i = 0; i < m_joint_refs.len(); ++i) {
    rv[i] = *m_joint_refs[i];
  }
  rv.last() = m_transform_matrix;
  return rv;
}
