#pragma once
#include <string>
#include <GL/gl3w.h>

enum uniformtype {
	UT_FLOAT,
	UT_MAT4,
	UT_SAMP2D
};


struct UniformData{
	uniformtype type;
	std::string name;
	unsigned int size;
	void* uniform_data;

};

struct texture_binding {
	std::string sampler2D;
	GLuint texture;
	GLuint texture_unit;
};

struct triangle_texture {
	GLuint texture_id;
	float texture_stride;
	float num_tris;
};

struct vertex {
	union {
		struct {
			float x, y, z;
		};
		float v[3];
	};
};

struct bvh_triangle {
	vertex v0;
	vertex v1;
	vertex v2;
	unsigned int index;

};

struct bvh_texture {
	GLuint texture_id;
	float texture_stride;
	float num_nodes;
};

struct aabb {
	vertex min;
	vertex max;
};

struct bvh_node {
	aabb bbox;
	bool isleaf = false;
	bvh_node* left;
	bvh_node* right;
	unsigned int triangle_idx = 0;

};


struct bvh_node_vec {
	aabb bbox;
	bool isleaf = false;
	unsigned int left;
	unsigned int right;
	unsigned int triangle_idx = 0;

};