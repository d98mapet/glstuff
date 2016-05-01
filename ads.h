#pragma once
#include <string>
#include <GL/gl3w.h>
#include <algorithm>


#include "tiny_obj_loader.h"


struct bvh_sorter {
	unsigned char axis;
	bool operator()(bvh_triangle a, bvh_triangle b) {
		float a_min;
		float b_min;
		if (axis == 0) {
			a_min = min(a.v0.x, min(a.v1.x, a.v2.x));
			b_min = min(b.v0.x, min(b.v1.x, b.v2.x));
		}
		else if (axis == 1) {
			a_min = min(a.v0.y, min(a.v1.y, a.v2.y));
			b_min = min(b.v0.y, min(b.v1.y, b.v2.y));

		}
		else {
			a_min = min(a.v0.z, min(a.v1.z, a.v2.z));
			b_min = min(b.v0.z, min(b.v1.z, b.v2.z));
		}
		return a_min < b_min;
	}
};

aabb calc_bounding_box(const std::vector<bvh_triangle>& triangles, unsigned int start, unsigned int end) {
	aabb bounding_box;
	bounding_box.min.x = FLT_MAX;
	bounding_box.min.y = FLT_MAX;
	bounding_box.min.z = FLT_MAX;
	bounding_box.max.x = -FLT_MAX;
	bounding_box.max.y = -FLT_MAX;
	bounding_box.max.z = -FLT_MAX;
	float min_x, min_y, min_z, max_x, max_y, max_z;
	bvh_triangle tri;
	for (unsigned int i = start; i <= end; i++) {
		tri = triangles[i];
		min_x = min(tri.v0.x, min(tri.v1.x, tri.v2.x));
		min_y = min(tri.v0.y, min(tri.v1.y, tri.v2.y));
		min_z = min(tri.v0.z, min(tri.v1.z, tri.v2.z));
		max_x = max(tri.v0.x, max(tri.v1.x, tri.v2.x));
		max_y = max(tri.v0.y, max(tri.v1.y, tri.v2.y));
		max_z = max(tri.v0.z, max(tri.v1.z, tri.v2.z));

		bounding_box.min.x = min(min_x, bounding_box.min.x);
		bounding_box.min.y = min(min_y, bounding_box.min.y);
		bounding_box.min.z = min(min_z, bounding_box.min.z);
		bounding_box.max.x = max(max_x, bounding_box.max.x);
		bounding_box.max.y = max(max_y, bounding_box.max.y);
		bounding_box.max.z = max(max_z, bounding_box.max.z);

	}
	min_x = min_x - 0.001f;
	min_y = min_y - 0.001f;
	min_z = min_z - 0.001f;
	max_x = max_x + 0.001f;
	max_y = max_y + 0.001f;
	max_z = max_z + 0.001f;
	return bounding_box;
}

unsigned char decide_split_plane(std::vector<bvh_triangle>& triangles, unsigned int start, unsigned end) {
	
	std::vector<float> axis_overlap;
	axis_overlap.resize(3);
	bvh_sorter sorter;
	float best_overlap = FLT_MAX;
	float overlap;
	unsigned char axis;
	for (unsigned char i = 0; i < 3; i++) {
		sorter.axis = i;
		std::sort(triangles.begin() + start, triangles.begin() + end, sorter);
		aabb bb1 = calc_bounding_box(triangles, start, start + (end - start) / 2);
		aabb bb2 = calc_bounding_box(triangles, (end - start) / 2 + 1 + start, end);
		if (bb1.max.v[i] < bb2.min.v[i]) {
			overlap = bb2.min.v[i] - bb1.max.v[i];
		}
		else {
			overlap = bb1.max.v[i] - bb2.min.v[i];
		}
		if (overlap < best_overlap) {
			axis = i;
			best_overlap = overlap;
		}
	}

	return axis;
	
}

void build_bvh_rec_vec(std::vector<bvh_triangle>& triangles, unsigned int start, unsigned int end, std::vector<bvh_node_vec>& tree) {

	if (start > end) {
		printf("oops!/n");
	}
	aabb bbox = calc_bounding_box(triangles, start, end);
	if ((end - start) < 1) {
		bvh_node_vec leaf_node;
		leaf_node.isleaf = true;
		leaf_node.triangle_idx = triangles[start].index;
		//printf("start %i, end %i, index %i\n", start, end, leaf_node.triangle_idx);
		leaf_node.left = leaf_node.right = 0xffffffff;
		leaf_node.bbox = bbox;
		tree.push_back(leaf_node);
		return;
	}
	bvh_node_vec node;
	node.bbox = bbox;
	node.triangle_idx = 0;
	node.isleaf = false;
	bvh_sorter sorter;
	tree.push_back(node);
	unsigned int current_idx = tree.size() - 1;
	float split = (float)(rand()) / (float)(RAND_MAX);
	sorter.axis = split<0.33f ? 0 : (split<0.67f ? 1 : 2);
	//std::sort(triangles.begin(), triangles.end(), sorter);
	sorter.axis = decide_split_plane(triangles, start, end);
	std::sort(triangles.begin() + start, triangles.begin() + end, sorter);
	build_bvh_rec_vec(triangles, start, start + (end - start) / 2, tree);
	tree[current_idx].left = current_idx + 1;
	tree[current_idx].right = tree.size();
	build_bvh_rec_vec(triangles, (end - start) / 2 + 1 + start, end, tree);
	//tree[current_idx].right = tree.size() - 1;
	return;
}

bvh_node* build_bvh_rec(std::vector<bvh_triangle>& triangles, unsigned int start, unsigned int end) {

	if (start > end) {
		printf("oops!/n");
	}
	aabb bbox = calc_bounding_box(triangles, start, end);
	if ((end - start) <= 1) {
		bvh_node* leaf_node = new bvh_node;
		leaf_node->isleaf = true;
		leaf_node->triangle_idx = triangles[start].index;
		leaf_node->left = leaf_node->right = NULL;
		leaf_node->bbox = bbox;
		return leaf_node;
	}
	bvh_node* node = new bvh_node;
	node->bbox = bbox;
	node->triangle_idx = 0;
	node->isleaf = false;
	bvh_sorter sorter;
	float split = (float)(rand() / RAND_MAX);
	sorter.axis = split<0.33f ? 0 : (split<0.67f ? 1 : 2);
	std::sort(triangles.begin(), triangles.end(), sorter);
	node->left = build_bvh_rec(triangles, start, start + (end - start) / 2);
	node->right = build_bvh_rec(triangles, (end - start) / 2 + 1 + start, end);
	return node;
}
/*
bvh_texture build_bvh_texture(const std::vector<bvh_node_vec>& tree) {

	//a node is aabb (6 floats)  triangle index (1 float, positive value == leaf) left/right index (2 floats)
	//stored as f32 rgb texture
	bvh_texture bvh_tex;
	bvh_tex.num_nodes = (float)tree.size();
	printf("%f, %f, %f", tree[0].bbox.min.x, tree[0].bbox.min.y, tree[0].bbox.min.z);
	unsigned int texture_x = (unsigned int)(sqrtf(bvh_tex.num_nodes*3.0f) + 1.0f);
	bvh_tex.texture_stride = 1.0f / (float)texture_x;
	std::vector<float> texture_data;
	texture_data.resize(texture_x*texture_x * 9, 0.0f);
	unsigned int dest = 0;
	for (unsigned int i = 0; i < tree.size(); i++) {
		texture_data[dest++] = tree[i].bbox.min.x;
		texture_data[dest++] = tree[i].bbox.min.y;
		texture_data[dest++] = tree[i].bbox.min.z;
		texture_data[dest++] = tree[i].bbox.max.x;
		texture_data[dest++] = tree[i].bbox.max.y;
		texture_data[dest++] = tree[i].bbox.max.z;
		texture_data[dest++] = tree[i].isleaf ? (float)tree[i].triangle_idx : -1.0f;
		texture_data[dest++] = (float)tree[i].left;
		texture_data[dest++] = (float)tree[i].right;
	}

	glGenTextures(1, &bvh_tex.texture_id);
	glBindTexture(GL_TEXTURE_2D, bvh_tex.texture_id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, texture_x, texture_x, 0, GL_RGB, GL_FLOAT, &texture_data[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return bvh_tex;
}
*/

bvh_texture build_bvh_texture(const std::vector<bvh_node_vec>& tree) {

	//a node is aabb (6 floats)  triangle index (1 float, positive value == leaf) left/right index (2 floats)
	//stored as f32 rgba texture "layout and compressed as bb1|bb2|left|right. If left == right => tri_index
	bvh_texture bvh_tex;
	bvh_tex.num_nodes = (float)tree.size();
	printf("%f, %f, %f", tree[0].bbox.min.x, tree[0].bbox.min.y, tree[0].bbox.min.z);
	unsigned int texture_x = (unsigned int)(sqrtf(bvh_tex.num_nodes*2.0f) + 1.0f);
	bvh_tex.texture_stride = 1.0f / (float)texture_x;
	std::vector<float> texture_data;
	texture_data.resize(texture_x*texture_x * 8, 0.0f);
	unsigned int dest = 0;
	for (unsigned int i = 0; i < tree.size(); i++) {
		texture_data[dest++] = tree[i].bbox.min.x;
		texture_data[dest++] = tree[i].bbox.min.y;
		texture_data[dest++] = tree[i].bbox.min.z;
		texture_data[dest++] = tree[i].bbox.max.x;
		texture_data[dest++] = tree[i].bbox.max.y;
		texture_data[dest++] = tree[i].bbox.max.z;
		texture_data[dest++] = tree[i].isleaf ? (float)tree[i].triangle_idx : (float)tree[i].left;;
		texture_data[dest++] = tree[i].isleaf ? (float)tree[i].triangle_idx : (float)tree[i].right;;
	}

	glGenTextures(1, &bvh_tex.texture_id);
	glBindTexture(GL_TEXTURE_2D, bvh_tex.texture_id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, texture_x, texture_x, 0, GL_RGBA, GL_FLOAT, &texture_data[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return bvh_tex;
}



bvh_texture build_bvh(const tinyobj::shape_t& shape) {
	unsigned int num_tris = shape.mesh.indices.size() / 3;
	std::vector<bvh_triangle> tri_list;
	tri_list.resize(num_tris);
	for (unsigned int i = 0; i < shape.mesh.indices.size(); i += 3) {
		unsigned int idx = shape.mesh.indices[i] * 3;
		bvh_triangle temp_tri;
		temp_tri.v0.x = shape.mesh.positions[idx + 0];
		temp_tri.v0.y = shape.mesh.positions[idx + 1];
		temp_tri.v0.z = shape.mesh.positions[idx + 2];
		idx = shape.mesh.indices[i + 1] * 3;
		temp_tri.v1.x = shape.mesh.positions[idx + 0];
		temp_tri.v1.y = shape.mesh.positions[idx + 1];
		temp_tri.v1.z = shape.mesh.positions[idx + 2];
		idx = shape.mesh.indices[i + 2] * 3;
		temp_tri.v2.x = shape.mesh.positions[idx + 0];
		temp_tri.v2.y = shape.mesh.positions[idx + 1];
		temp_tri.v2.z = shape.mesh.positions[idx + 2];
		temp_tri.index = i / 3;
		tri_list[i / 3] = temp_tri;
	}
	//bvh_node* root = build_bvh_rec(tri_list, 0, tri_list.size()-1);
	//return root;
	std::vector<bvh_node_vec> tree;
	build_bvh_rec_vec(tri_list, 0, tri_list.size() - 1, tree);
	return build_bvh_texture(tree);
}

void combine_shapes(const std::vector<tinyobj::shape_t>& shapes, tinyobj::shape_t& shape) {
	shape.name = "combineshape";
	unsigned int index;
	unsigned int offset = 0;

	for (unsigned int i = 0; i < shapes.size(); i++) {
		bool has_normals = shapes[i].mesh.normals.size() > 0;
		bool has_uvs = shapes[i].mesh.texcoords.size() > 0;
		
		float r = (float)(rand()) / (float)(RAND_MAX);
		float g = (float)(rand()) / (float)(RAND_MAX);
		float b = (float)(rand()) / (float)(RAND_MAX);

		for (unsigned int j = 0; j < shapes[i].mesh.indices.size(); j += 3) {

			index = shapes[i].mesh.indices[j] * 3;
			shape.mesh.indices.push_back(j + offset);
			shape.mesh.positions.push_back(shapes[i].mesh.positions[index]);
			shape.mesh.positions.push_back(shapes[i].mesh.positions[index + 1]);
			shape.mesh.positions.push_back(shapes[i].mesh.positions[index + 2]);
			shape.mesh.normals.push_back(has_normals ? shapes[i].mesh.normals[index] : r);
			shape.mesh.normals.push_back(has_normals ? shapes[i].mesh.normals[index + 1] : g);
			shape.mesh.normals.push_back(has_normals ? shapes[i].mesh.normals[index + 2] : b);
			shape.mesh.texcoords.push_back(has_uvs ? shapes[i].mesh.texcoords[0] : 0.0f);
			shape.mesh.texcoords.push_back(has_uvs ? shapes[i].mesh.texcoords[0 + 1] : 0.0f);

			index = shapes[i].mesh.indices[j + 1] * 3;
			shape.mesh.indices.push_back(j + 1 + offset);
			shape.mesh.positions.push_back(shapes[i].mesh.positions[index]);
			shape.mesh.positions.push_back(shapes[i].mesh.positions[index + 1]);
			shape.mesh.positions.push_back(shapes[i].mesh.positions[index + 2]);
			shape.mesh.normals.push_back(has_normals ? shapes[i].mesh.normals[index] : 0.0f);
			shape.mesh.normals.push_back(has_normals ? shapes[i].mesh.normals[index + 1] : 1.0f);
			shape.mesh.normals.push_back(has_normals ? shapes[i].mesh.normals[index + 2] : 0.0f);
			shape.mesh.texcoords.push_back(has_uvs ? shapes[i].mesh.texcoords[0] : 0.0f);
			shape.mesh.texcoords.push_back(has_uvs ? shapes[i].mesh.texcoords[0 + 1] : 0.0f);

			index = shapes[i].mesh.indices[j + 2] * 3;
			shape.mesh.indices.push_back(j + 2 + offset);
			shape.mesh.positions.push_back(shapes[i].mesh.positions[index]);
			shape.mesh.positions.push_back(shapes[i].mesh.positions[index + 1]);
			shape.mesh.positions.push_back(shapes[i].mesh.positions[index + 2]);
			shape.mesh.normals.push_back(has_normals ? shapes[i].mesh.normals[index] : 0.0f);
			shape.mesh.normals.push_back(has_normals ? shapes[i].mesh.normals[index + 1] : 1.0f);
			shape.mesh.normals.push_back(has_normals ? shapes[i].mesh.normals[index + 2] : 0.0f);
			shape.mesh.texcoords.push_back(has_uvs ? shapes[i].mesh.texcoords[0] : 0.0f);
			shape.mesh.texcoords.push_back(has_uvs ? shapes[i].mesh.texcoords[0 + 1] : 0.0f);


		}
		offset += shapes[i].mesh.indices.size();
	}
}

triangle_texture build_triangle_texture(const tinyobj::shape_t& shape) {
	//Extract data from shape object. Build a non indexed list
	//containing triangle data to be used in shader.
	//Contains position (3 floats) Normals (3 floats) UV (2 floats)
	//So each vertex is represented using 8 floats
	//px,py,px,u,nx,ny,nz,v
	//using f32 rgba texture
	triangle_texture tex;
	//	bvh_node* root = build_bvh(shape);
	unsigned int num_tris = shape.mesh.indices.size() / 3;
	tex.num_tris = (float)num_tris;
	std::vector<float> texture_data;
	//Make a square texture;
	unsigned int texture_x = (unsigned int)(sqrtf(num_tris*6.0f) + 1.0f);
	float texture_stride = 1.0f / texture_x;
	texture_data.resize(texture_x*texture_x*4, 0.0f);
	tex.texture_stride = 1.0f / texture_x;
	bool has_normals = shape.mesh.normals.size() > 0;
	bool has_uvs = shape.mesh.texcoords.size() > 0;
	unsigned int dest = 0;
	for (unsigned int i = 0; i < shape.mesh.indices.size(); i += 3) {
		//v0
		unsigned int idx = shape.mesh.indices[i] * 3;
		texture_data[dest++] = shape.mesh.positions[idx + 0];
		texture_data[dest++] = shape.mesh.positions[idx + 1];
		texture_data[dest++] = shape.mesh.positions[idx + 2];
		texture_data[dest++] = has_uvs ? shape.mesh.texcoords[shape.mesh.indices[i] * 2 + 0] : 0.0f;

		texture_data[dest++] = has_normals ? shape.mesh.normals[idx + 0] : 0.0f;
		texture_data[dest++] = has_normals ? shape.mesh.normals[idx + 1] : 1.0f;
		texture_data[dest++] = has_normals ? shape.mesh.normals[idx + 2] : 0.0f;
		texture_data[dest++] = has_uvs ? shape.mesh.texcoords[shape.mesh.indices[i] * 2 + 1] : 0.0f;
		//v1
		idx = shape.mesh.indices[i + 1] * 3;
		texture_data[dest++] = shape.mesh.positions[idx + 0];
		texture_data[dest++] = shape.mesh.positions[idx + 1];
		texture_data[dest++] = shape.mesh.positions[idx + 2];
		texture_data[dest++] = has_uvs ? shape.mesh.texcoords[shape.mesh.indices[i + 1] * 2 + 0] : 0.0f;

		texture_data[dest++] = has_normals ? shape.mesh.normals[idx + 0] : 0.0f;
		texture_data[dest++] = has_normals ? shape.mesh.normals[idx + 1] : 1.0f;
		texture_data[dest++] = has_normals ? shape.mesh.normals[idx + 2] : 0.0f;
		texture_data[dest++] = has_uvs ? shape.mesh.texcoords[shape.mesh.indices[i + 1] * 2 + 1] : 0.0f;
		//v2
		idx = shape.mesh.indices[i + 2] * 3;
		texture_data[dest++] = shape.mesh.positions[idx + 0];
		texture_data[dest++] = shape.mesh.positions[idx + 1];
		texture_data[dest++] = shape.mesh.positions[idx + 2];
		texture_data[dest++] = has_uvs ? shape.mesh.texcoords[shape.mesh.indices[i + 2] * 2 + 0] : 0.0f;

		texture_data[dest++] = has_normals ? shape.mesh.normals[idx + 0] : 0.0f;
		texture_data[dest++] = has_normals ? shape.mesh.normals[idx + 1] : 1.0f;
		texture_data[dest++] = has_normals ? shape.mesh.normals[idx + 2] : 0.0f;
		texture_data[dest++] = has_uvs ? shape.mesh.texcoords[shape.mesh.indices[i + 2] * 2 + 1] : 0.0f;

	}
	GLuint tri_tex;
	glGenTextures(1, &tri_tex);
	glBindTexture(GL_TEXTURE_2D, tri_tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, texture_x, texture_x, 0, GL_RGBA, GL_FLOAT, &texture_data[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	tex.texture_id = tri_tex;
	return tex;

}


bool load_obj(std::string file, std::vector<triangle_texture>& tri_textures, std::vector<bvh_texture>& bvh_textures) {

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	bool success;
	std::string error;
	success = tinyobj::LoadObj(shapes, materials, error, file.c_str()); //Always triangulate;

	if (!success) {
		//file could not load.
		return false;
	}
	if (!error.empty()) {
		//File loaded with warnings
	}
	tinyobj::shape_t cshape;
	combine_shapes(shapes, cshape);
	triangle_texture tri_tex = build_triangle_texture(cshape);
	tri_textures.push_back(tri_tex);
	bvh_texture bvh_tex = build_bvh(cshape);
	bvh_textures.push_back(bvh_tex);
	return true;
}
