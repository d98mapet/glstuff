#version 330 core
in vec2 uv;

//out vec3 color;
layout(location = 0) out vec3 color;




uniform float iGlobalTime;
uniform sampler2D input_map;
uniform sampler2D color_map; // path(../../red.png)
uniform sampler2D height_map; // path(d:/shaders/clouds.png)

uniform sampler2D triangle_texture;
uniform float triangle_texture_stride;
uniform float num_tris;

uniform sampler2D bvh_texture;
uniform float bvh_texture_stride;
uniform float num_nodes;
uniform float test_float=1.0; //ui(0.0, 10.0);

mat3 setCamera( in vec3 ro, in vec3 ta, float cr )
{
	vec3 cw = normalize(ta-ro);
	vec3 cp = vec3(sin(cr), cos(cr),0.0);
	vec3 cu = normalize( cross(cw,cp) );
	vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}





float seed = iGlobalTime+uv.x+uv.y;
//float seed = iGlobalTime;//+uv.x+uv.y;

vec2 hash2() {
    return fract(sin(vec2(seed+=0.1,seed+=0.1))*vec2(43758.5453123,22578.1459123));
//    return fract(sin(vec2(seed,seed))*vec2(43758.5453123,22578.1459123));
}

vec3 randomSphereDirection() {
    vec2 r = hash2()*6.2831;
    vec3 dr=vec3(sin(r.x)*vec2(sin(r.y),cos(r.y)),cos(r.x));
    return dr;
}

vec3 randomHemisphereDirection( const vec3 n ) {
    vec3 dr = randomSphereDirection();
    return dot(dr,n) * dr;
}

vec3 cosWeightedRandomHemisphereDirection( const vec3 n ) {
    vec2 r = hash2();
    
    vec3  uu = normalize( cross( n, vec3(0.0,1.0,1.0) ) );
    vec3  vv = cross( uu, n );
    
    float ra = sqrt(r.y);
    float rx = ra*cos(6.2831*r.x); 
    float ry = ra*sin(6.2831*r.x);
    float rz = sqrt( 1.0-r.y );
    vec3  rr = vec3( rx*uu + ry*vv + rz*n );
    //vec3  rr = vec3( uu + vv + n );
    return normalize( rr );
}





bool intersect_aabb(vec3 ray_origin, vec3 ray_dir, vec3 bb_min, vec3 bb_max, out float t0, out float t1) {
    vec3 invR = 1.0 / ray_dir;
    vec3 tbot = invR * (bb_min-ray_origin);
    vec3 ttop = invR * (bb_max-ray_origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);
    return t0 <= t1;
}



vec2 calc_index(float triangle, float vertex, float component) {
        float index_sum = triangle * 6.0*triangle_texture_stride +
                        vertex*2.0*triangle_texture_stride +
                        component * triangle_texture_stride;
        vec2 index;
        index.x = fract(index_sum) + triangle_texture_stride*0.5;
        index.y = floor(index_sum)*triangle_texture_stride + triangle_texture_stride*0.5;
        return index;
}
/*
vec2 calc_index_bvh(float node, float component) {
        float index_sum = node * 3.0*bvh_texture_stride +
                        component * bvh_texture_stride;
        vec2 index;
        index.x = fract(index_sum) + bvh_texture_stride*0.5;
        index.y = floor(index_sum)*bvh_texture_stride + bvh_texture_stride*0.5;
        return index;
}
*/
vec2 calc_index_bvh(float node, float component) {
        float index_sum = node * 2.0*bvh_texture_stride +
                        component * bvh_texture_stride;
        vec2 index;
        index.x = fract(index_sum) + bvh_texture_stride*0.5;
        index.y = floor(index_sum)*bvh_texture_stride + bvh_texture_stride*0.5;
        return index;
}

vec4 intersect_tri(vec3 v0, vec3 v1, vec3 v2, vec3 ro, vec3 rd) {
    vec3 p0 = v0;
    vec3 e0 = v1 - v0;
    vec3 e1 = v2 - v0;
    vec3 pv = cross(rd, e1);

    float det = dot(e0, pv);
    //if (det > 1e-10) {
        vec3 tv = ro - p0;
        vec3 qv = cross(tv, e0);

        vec4 uvt;
        uvt.x = dot(tv, pv);
        uvt.y = dot(rd, qv);
        uvt.z = dot(e1, qv);
        uvt.xyz = uvt.xyz / det;
        uvt.w = 1.0 - uvt.x - uvt.y;

        if (all(greaterThanEqual(uvt, vec4(0.0)))) {
            return uvt;
        }
    //}
    return vec4(0.0);
}

struct intersection {
    vec3 pos;
    vec3 nor;
    vec2 uv;
    vec3 mat;
    float t;
};

vec3 checker(vec2 uvc) {

    float size = 2.0;
    float sum = floor(size*uvc.x) + floor(size*uvc.y);
    bool even = mod(sum,2.0) == 0.0;
    return (even?vec3(0.9,0.9,0.9):vec3(0.0,0.0,0.9));
}
vec3 checker3d(vec3 pos) {

    float size = 2.0;
    float sum = floor(size*pos.x) + floor(size*pos.y)+ floor(size*pos.z);
    bool even = mod(sum,2.0) == 0.0;
   

    return (even?vec3(0.9,0.9,0.9):vec3(0.0,0.0,0.9));
}
intersection calc_intersection(vec4 raw_intersection) {

    vec3 bary = vec3(raw_intersection.x, raw_intersection.y, 1.0-raw_intersection.x-raw_intersection.y);

    float node = raw_intersection.w;

    vec4 v0 = texture2D(triangle_texture, calc_index(node, 0.0, 0.0)).rgba;
    vec4 v1 = texture2D(triangle_texture, calc_index(node, 1.0, 0.0)).rgba;
    vec4 v2 = texture2D(triangle_texture, calc_index(node, 2.0, 0.0)).rgba;

    vec4 n0 = texture2D(triangle_texture, calc_index(node, 0.0, 1.0)).rgba;
    vec4 n1 = texture2D(triangle_texture, calc_index(node, 1.0, 1.0)).rgba;
    vec4 n2 = texture2D(triangle_texture, calc_index(node, 2.0, 1.0)).rgba;

    vec2 uv0 = vec2(v0.a, n0.a);
    vec2 uv1 = vec2(v1.a, n1.a);
    vec2 uv2 = vec2(v2.a, n2.a);
    vec3 fnorm = normalize(cross(v1.xyz-v0.xyz, v2.xyz-v0.xyz));
    bary = bary.yzx;
    intersection inter;
    inter.pos = v2.xyz * bary.x + v0.xyz*bary.y +v1.xyz * bary.z; 
    inter.nor = fnorm,//n0.xyz;normalize(n2.xyz * bary.x + n0.xyz*bary.y +n1.xyz * bary.z);
    inter.uv = uv2.xy * bary.x + uv0.xy*bary.y +uv1.xy * bary.z;
    inter.t = raw_intersection.z;
    inter.mat = checker3d(inter.pos);//vec3(inter.uv,0);
    return inter;

}

/*
float intersect_bvh_tree_shadow_3(vec3 ro, vec3 rd) {
    vec4 uvt;

    float[20] stack;
    int stack_p = 0;
    stack[stack_p++] = 0.0;

    float node;
    vec3 bb_min;
    vec3 bb_max;
    vec3 node_data;
    vec3 child_node_data;
    vec3 childl;
    vec3 childr;
    float t0, t1;
    vec4 best = vec4(0.0, 0.0, 10000000.0, 0.0);
    bool intersect_l;
    bool intersect_r;    
    bool leaf_l;
    bool leaf_r;    
    bool traverse_l;
    bool traverse_r;
    node = 0.0;
    vec3 v0, v1, v2;
    //vec4 uvt;    

    while (stack_p < 20) {
        //node = stack[--stack_p];

        leaf_l = false;
        leaf_r = false;
        node_data = texture2D(bvh_texture, calc_index_bvh(node, 2.0)).rgb;
        //childl = texture2D(bvh_texture, calc_index_bvh(node_data.y, 0.0)).rgb;
        //childr = texture2D(bvh_texture, calc_index_bvh(node_data.z, 1.0)).rgb;
        //Check left node
        bb_min = texture2D(bvh_texture, calc_index_bvh(node_data.y, 0.0)).rgb;
        bb_max = texture2D(bvh_texture, calc_index_bvh(node_data.y, 1.0)).rgb;
        intersect_l = intersect_aabb(ro, rd, bb_min, bb_max, t0, t1);

        bb_min = texture2D(bvh_texture, calc_index_bvh(node_data.z, 0.0)).rgb;
        bb_max = texture2D(bvh_texture, calc_index_bvh(node_data.z, 1.0)).rgb;
        intersect_r = intersect_aabb(ro, rd, bb_min, bb_max, t0, t1);

        if (intersect_l) {
            child_node_data = texture2D(bvh_texture, calc_index_bvh(node_data.y, 2.0)).rgb;

            if (child_node_data.x > 0.0) { //node is leaf, intersect tri
                v0 = texture2D(triangle_texture, calc_index(child_node_data.x, 0.0, 0.0)).rgb;
                v1 = texture2D(triangle_texture, calc_index(child_node_data.x, 1.0, 0.0)).rgb;
                v2 = texture2D(triangle_texture, calc_index(child_node_data.x, 2.0, 0.0)).rgb;
                uvt = intersect_tri(v0, v1, v2, ro, rd);
                if ((uvt.z > 0) ) {
                    return 0.0;
                }
                leaf_l = true;
            }

        }

        if (intersect_r) {
            child_node_data = texture2D(bvh_texture, calc_index_bvh(node_data.z, 2.0)).rgb;

            if (child_node_data.x > 0.0) { //node is leaf, intersect tri
                v0 = texture2D(triangle_texture, calc_index(child_node_data.x, 0.0, 0.0)).rgb;
                v1 = texture2D(triangle_texture, calc_index(child_node_data.x, 1.0, 0.0)).rgb;
                v2 = texture2D(triangle_texture, calc_index(child_node_data.x, 2.0, 0.0)).rgb;
                uvt = intersect_tri(v0, v1, v2, ro, rd);
                if ((uvt.z > 0) ) {
                    return 0.0;
                }
                leaf_r = true;
            }
            
        }

        traverse_l = intersect_l && !leaf_l;
        traverse_r = intersect_r && !leaf_r;

        if (!traverse_l && !traverse_r) {
            node = stack[--stack_p];
        } else {
            node = traverse_l ? node_data.y : node_data.z;
            if (traverse_l && traverse_r) {
                stack[stack_p++] = node_data.z;
            }
        }



        if (stack_p == 0) {
 
                return 1.0;
           
        }

    }
    return 0.0;
}
*/


/*
bool intersect_bvh_tree_3(vec3 ro, vec3 rd, out intersection inter) {
    vec4 uvt;

    float[10] stack;
    int stack_p = 0;
    stack[stack_p++] = 0.0;

    float node;
    vec3 bb_min;
    vec3 bb_max;
    vec3 node_data;
    vec3 child_node_data;
    vec3 childl;
    vec3 childr;
    float t0, t1;
    vec4 best = vec4(0.0, 0.0, 10000000.0, 0.0);
    bool intersect_l;
    bool intersect_r;    
    bool leaf_l;
    bool leaf_r;    
    bool traverse_l;
    bool traverse_r;
    node = 0.0;
    vec3 v0, v1, v2;
    //vec4 uvt;    

    while (stack_p < 20.0) {
        //node = stack[--stack_p];

        leaf_l = false;
        leaf_r = false;
        node_data = texture2D(bvh_texture, calc_index_bvh(node, 2.0)).rgb;
        //childl = texture2D(bvh_texture, calc_index_bvh(node_data.y, 0.0)).rgb;
        //childr = texture2D(bvh_texture, calc_index_bvh(node_data.z, 1.0)).rgb;
        //Check left node
        bb_min = texture2D(bvh_texture, calc_index_bvh(node_data.y, 0.0)).rgb;
        bb_max = texture2D(bvh_texture, calc_index_bvh(node_data.y, 1.0)).rgb;
        intersect_l = intersect_aabb(ro, rd, bb_min, bb_max, t0, t1);

        bb_min = texture2D(bvh_texture, calc_index_bvh(node_data.z, 0.0)).rgb;
        bb_max = texture2D(bvh_texture, calc_index_bvh(node_data.z, 1.0)).rgb;
        intersect_r = intersect_aabb(ro, rd, bb_min, bb_max, t0, t1);

        if (intersect_l) {
            child_node_data = texture2D(bvh_texture, calc_index_bvh(node_data.y, 2.0)).rgb;

            if (child_node_data.x > 0.0) { //node is leaf, intersect tri
                v0 = texture2D(triangle_texture, calc_index(child_node_data.x, 0.0, 0.0)).rgb;
                v1 = texture2D(triangle_texture, calc_index(child_node_data.x, 1.0, 0.0)).rgb;
                v2 = texture2D(triangle_texture, calc_index(child_node_data.x, 2.0, 0.0)).rgb;
                uvt = intersect_tri(v0, v1, v2, ro, rd);
                if ((uvt.z > 0) && (uvt.z < best.z)) {
                    best = uvt;
                    best.w = child_node_data.x;
                }
                leaf_l = true;
            }

        }

        if (intersect_r) {
            child_node_data = texture2D(bvh_texture, calc_index_bvh(node_data.z, 2.0)).rgb;

            if (child_node_data.x > 0.0) { //node is leaf, intersect tri
                v0 = texture2D(triangle_texture, calc_index(child_node_data.x, 0.0, 0.0)).rgb;
                v1 = texture2D(triangle_texture, calc_index(child_node_data.x, 1.0, 0.0)).rgb;
                v2 = texture2D(triangle_texture, calc_index(child_node_data.x, 2.0, 0.0)).rgb;
                uvt = intersect_tri(v0, v1, v2, ro, rd);
                if ((uvt.z > 0) && (uvt.z < best.z)) {
                    best = uvt;
                    best.w = child_node_data.x;
                }
                leaf_r = true;
            }
            
        }

        traverse_l = intersect_l && !leaf_l;
        traverse_r = intersect_r && !leaf_r;

        if (!traverse_l && !traverse_r) {
            node = stack[--stack_p];
        } else {
            node = traverse_l ? node_data.y : node_data.z;
            if (traverse_l && traverse_r) {
                stack[stack_p++] = node_data.z;
            }
        }



        if (stack_p == 0.0) {
            if (best.z < 10000000.0) {
                inter = calc_intersection(best);
                return true;
            } else {
                return false;
            }
        }

    }
    return false;
}
*/



bool intersect_bvh_tree_3(vec3 ro, vec3 rd, out intersection inter) {
    vec4 uvt;

    float[20] stack;
    int stack_p = 0;
    stack[stack_p++] = 0.0;

    float node;
    vec3 bb_min;
    vec3 bb_max;
    vec3 node_data;
    float t0, t1;
    vec4 best = vec4(0.0, 0.0, 10000000.0, 0.0);
    bool intersect_l;
    bool intersect_r;    
    bool leaf_l;
    bool leaf_r;    
    bool traverse_l;
    bool traverse_r;
    node = 0.0;
    vec3 v0, v1, v2;
     
    vec4 data1, data2, data3, data4;



    while (stack_p < 20.0) {
        //node = stack[--stack_p];

        leaf_l = false;
        leaf_r = false;
        node_data = texture2D(bvh_texture, calc_index_bvh(node, 1.0)).gba;

        data1 = texture2D(bvh_texture, calc_index_bvh(node_data.y, 0.0)).rgba;
        data2 = texture2D(bvh_texture, calc_index_bvh(node_data.y, 1.0)).rgba;

        data3 = texture2D(bvh_texture, calc_index_bvh(node_data.z, 0.0)).rgba;
        data4 = texture2D(bvh_texture, calc_index_bvh(node_data.z, 1.0)).rgba;
        
        bb_min = vec3(data1.xyz);//texture2D(bvh_texture, calc_index_bvh(node_data.y, 0.0)).rgb;
        bb_max = vec3(data1.w, data2.xy);//texture2D(bvh_texture, calc_index_bvh(node_data.y, 1.0)).rgb;
        intersect_l = intersect_aabb(ro, rd, bb_min, bb_max, t0, t1);

        bb_min = vec3(data3.xyz);//texture2D(bvh_texture, calc_index_bvh(node_data.z, 0.0)).rgb;
        bb_max = vec3(data3.w, data4.xy);//texture2D(bvh_texture, calc_index_bvh(node_data.z, 1.0)).rgb;
        intersect_r = intersect_aabb(ro, rd, bb_min, bb_max, t0, t1);

        if (intersect_l) {

            if (data2.z == data2.w) { //node is leaf, intersect tri
                v0 = texture2D(triangle_texture, calc_index(data2.z, 0.0, 0.0)).rgb;
                v1 = texture2D(triangle_texture, calc_index(data2.z, 1.0, 0.0)).rgb;
                v2 = texture2D(triangle_texture, calc_index(data2.z, 2.0, 0.0)).rgb;
                uvt = intersect_tri(v0, v1, v2, ro, rd);
                if ((uvt.z > 0) && (uvt.z < best.z)) {
                    best = uvt;
                    best.w = data2.z;
                }
                leaf_l = true;
            }

        }

        if (intersect_r) {
 
            if (data4.z == data4.w) { //node is leaf, intersect tri
                v0 = texture2D(triangle_texture, calc_index(data4.z, 0.0, 0.0)).rgb;
                v1 = texture2D(triangle_texture, calc_index(data4.z, 1.0, 0.0)).rgb;
                v2 = texture2D(triangle_texture, calc_index(data4.z, 2.0, 0.0)).rgb;
                uvt = intersect_tri(v0, v1, v2, ro, rd);
                if ((uvt.z > 0) && (uvt.z < best.z)) {
                    best = uvt;
                    best.w = data4.z;
                }
                leaf_r = true;
            }
            
        }

        traverse_l = intersect_l && !leaf_l;
        traverse_r = intersect_r && !leaf_r;

        if (!traverse_l && !traverse_r) {
            node = stack[--stack_p];
        } else {
            node = traverse_l ? node_data.y : node_data.z;
            if (traverse_l && traverse_r) {
                stack[stack_p++] = node_data.z;
            }
        }



        if (stack_p == 0.0) {
            if (best.z < 10000000.0) {
                inter = calc_intersection(best);
                return true;
            } else {
                return false;
            }
        }

    }
    return false;
}


float intersect_bvh_tree_shadow_3(vec3 ro, vec3 rd) {
    vec4 uvt;

    float[20] stack;
    int stack_p = 0;
    stack[stack_p++] = 0.0;

    float node;
    vec3 bb_min;
    vec3 bb_max;
    vec3 node_data;
   float t0, t1;
    bool intersect_l;
    bool intersect_r;    
    bool leaf_l;
    bool leaf_r;    
    bool traverse_l;
    bool traverse_r;
    node = 0.0;
    vec3 v0, v1, v2;
   vec4 data1, data2, data3, data4;



    while (stack_p < 20.0) {

        leaf_l = false;
        leaf_r = false;
        node_data = texture2D(bvh_texture, calc_index_bvh(node, 1.0)).gba;
  
        data1 = texture2D(bvh_texture, calc_index_bvh(node_data.y, 0.0)).rgba;
        data2 = texture2D(bvh_texture, calc_index_bvh(node_data.y, 1.0)).rgba;

        data3 = texture2D(bvh_texture, calc_index_bvh(node_data.z, 0.0)).rgba;
        data4 = texture2D(bvh_texture, calc_index_bvh(node_data.z, 1.0)).rgba;
        
        bb_min = vec3(data1.xyz);//texture2D(bvh_texture, calc_index_bvh(node_data.y, 0.0)).rgb;
        bb_max = vec3(data1.w, data2.xy);//texture2D(bvh_texture, calc_index_bvh(node_data.y, 1.0)).rgb;
        intersect_l = intersect_aabb(ro, rd, bb_min, bb_max, t0, t1);

        bb_min = vec3(data3.xyz);//texture2D(bvh_texture, calc_index_bvh(node_data.z, 0.0)).rgb;
        bb_max = vec3(data3.w, data4.xy);//texture2D(bvh_texture, calc_index_bvh(node_data.z, 1.0)).rgb;
        intersect_r = intersect_aabb(ro, rd, bb_min, bb_max, t0, t1);

        if (intersect_l) {

            if (data2.z == data2.w) { //node is leaf, intersect tri
                v0 = texture2D(triangle_texture, calc_index(data2.z, 0.0, 0.0)).rgb;
                v1 = texture2D(triangle_texture, calc_index(data2.z, 1.0, 0.0)).rgb;
                v2 = texture2D(triangle_texture, calc_index(data2.z, 2.0, 0.0)).rgb;
                uvt = intersect_tri(v0, v1, v2, ro, rd);
                if ((uvt.z > 0) ) {
                    return 0.0;
                }
                leaf_l = true;
            }

        }

        if (intersect_r) {

            if (data4.z == data4.w) { //node is leaf, intersect tri
                v0 = texture2D(triangle_texture, calc_index(data4.z, 0.0, 0.0)).rgb;
                v1 = texture2D(triangle_texture, calc_index(data4.z, 1.0, 0.0)).rgb;
                v2 = texture2D(triangle_texture, calc_index(data4.z, 2.0, 0.0)).rgb;
                uvt = intersect_tri(v0, v1, v2, ro, rd);
                if ((uvt.z > 0) ) {
                    return 0.0;
                }
                leaf_r = true;
            }
            
        }

        traverse_l = intersect_l && !leaf_l;
        traverse_r = intersect_r && !leaf_r;

        if (!traverse_l && !traverse_r) {
            node = stack[--stack_p];
        } else {
            node = traverse_l ? node_data.y : node_data.z;
            if (traverse_l && traverse_r) {
                stack[stack_p++] = node_data.z;
            }
        }



        if (stack_p == 0.0) {
             return 1.0;
        }

    }
    return 0.0;
}



uniform float ls = 1.0; //ui(1.0, 5.0)
uniform float dof = 10.0; //ui(1, 20)
uniform float x_focal = 0.0; //ui(-10, 10)

void main()
{
 

 
	color = vec3(0.0);

    vec3 lightPos = vec3(0.0, 20.0*sin(test_float), 3.0);

    vec3 light_size = vec3(hash2().x,hash2().x, hash2().x);
    normalize(light_size);
    lightPos += light_size / ls;
    vec3 ro = vec3( -16.0, 6.5, 0.0);
    vec3 ta = vec3( x_focal, 6.5, 0.0);//cos(iGlobalTime), sin(iGlobalTime) );
    vec2 p = -1.0+2.0*uv;

    vec3 v0 = vec3(0.0, -0.5, -0.5);
    vec3 v1 = vec3(0.0, 0.5, 0.5);
    vec3 v2 = vec3(0.0, 0.5, -0.5);
    
 
    seed = p.x + p.y * 3.43121412313 + iGlobalTime;
  
    ro.yz += +hash2()/dof;
    mat3 cam_to_world = setCamera(ro, ta, 0.0);
    vec3 rd = cam_to_world * normalize( vec3(p.xy,1.0) );
    rd = normalize(rd);
    float s = 0.0;
    vec3 o_ro = ro;
    vec3 o_rd = rd;
    color = vec3(0.0,0.0,0.0);


    intersection inter;

    float d = 0.0;
    ro = o_ro;
    rd = o_rd;
    vec3 mask = vec3(1.0);
    for (int depth=0; depth<2; depth++) {


        if (intersect_bvh_tree_3(ro, rd, inter)) {


           

            vec3 lightDir = lightPos-inter.pos;
            float light_dist = length(lightDir);
            lightDir = normalize(lightDir);

            float in_shadow = intersect_bvh_tree_shadow_3(inter.pos+inter.nor*.01, lightDir);
            float diffuse = dot(lightDir, inter.nor)*1.9;
            //in_shadow = 1.0;

            vec3 lc; 
            
            lc = diffuse*inter.mat*in_shadow;// * tex3D(color_map, inter, normal);
                //if (depth>0)
            color += mask*lc;
             mask *= inter.mat;
            
       
            ro = inter.pos+inter.nor*.02;
            //rd = normalize(cosWeightedRandomHemisphereDirection(inter.nor));
            rd = normalize(randomHemisphereDirection(inter.nor));
            d++;

        } else {
            color += vec3(0.7 )*mask;
            d++;
            //s++;
            break;
        }
        
    } 
 
    
    if (iGlobalTime>0.0) {
        color = (color + (texture(input_map, uv).rgb));
        //color = ((texture(input_map, uv).rgb));
    }
    
}

//Good debug code.

/*
vec4 intersect_bvh_list(vec3 ro, vec3 rd) {
    float cnt = 0.0;
    //vec4 best;
   vec4 best = vec4(0.0, 0.0, 10000000.0, 0.0);
 
    while (cnt<num_nodes) {
        vec2 buv = calc_index_bvh(cnt, 2);
        vec3 node_data = texture2D(bvh_texture, buv).rgb;
        vec4 uvt;
        if (node_data.x > 0.0) {
            vec3 v0 = texture2D(triangle_texture, calc_index(node_data.r, 0.0, 0.0).xy).rgb;
            vec3 v1 = texture2D(triangle_texture, calc_index(node_data.r, 1.0, 0.0).xy).rgb;
            vec3 v2 = texture2D(triangle_texture, calc_index(node_data.r, 2.0, 0.0).xy).rgb;
            vec4 uvt = intersect_tri(v0, v1, v2, ro, rd);
            if ((uvt.z > 0) && (uvt.z < best.z)) {
                best = uvt;
            }
            //
            //vec3 bb_min = texture2D(bvh_texture, calc_index_bvh(cnt, 0.0)).rgb;
            //vec3 bb_max = texture2D(bvh_texture, calc_index_bvh(cnt, 1.0)).rgb;
            //float t0, t1;
            //if (intersect_aabb(ro, rd, bb_min, bb_max, t0, t1) ) {
            //    best.z = cnt;
            //}
            //
        }
        cnt++;
    }
    return best;
}

vec4 intersect_triangle_list(vec3 ro, vec3 rd) {
    float i = 0;
    vec2 index = vec2(0.0, 0.0);
    vec4 best = vec4(0.0, 0.0, 10000000.0, 0.0);
    while (i<num_tris) {
        vec3 v0 = texture2D(triangle_texture, calc_index(i, 0.0, 0.0).xy).rgb;
        vec3 v1 = texture2D(triangle_texture, calc_index(i, 1.0, 0.0).xy).rgb;
        vec3 v2 = texture2D(triangle_texture, calc_index(i, 2.0, 0.0).xy).rgb;
        vec4 uvt = intersect_tri(v0, v1, v2, ro, rd);
        if ((uvt.z > 0) && (uvt.z < best.z)) {
            best = uvt;
        }

        i+=1.0;
    }
    return best;
}


float intersect_bvh_tree_shadow(vec3 ro, vec3 rd) {
    vec4 uvt;

    float[18] stack;
    int stack_p = 0;
    stack[stack_p++] = 0.0;

    float node;
    vec3 bb_min;
    vec3 bb_max;
    vec3 node_data;
    float t0, t1;
    vec4 best = vec4(0.0, 0.0, 10000000.0, 0.0);
 
    while (stack_p < 20.0) {
        node = stack[--stack_p];

        bb_min = texture2D(bvh_texture, calc_index_bvh(node, 0.0)).rgb;
        bb_max = texture2D(bvh_texture, calc_index_bvh(node, 1.0)).rgb;
        if (intersect_aabb(ro, rd, bb_min, bb_max, t0, t1) ) {
            node_data = texture2D(bvh_texture, calc_index_bvh(node, 2.0)).rgb;
            if (node_data.x > 0.0) { //node is leaf, intersect tri
                vec3 v0 = texture2D(triangle_texture, calc_index(node_data.x, 0.0, 0.0)).rgb;
                vec3 v1 = texture2D(triangle_texture, calc_index(node_data.x, 1.0, 0.0)).rgb;
                vec3 v2 = texture2D(triangle_texture, calc_index(node_data.x, 2.0, 0.0)).rgb;
                vec4 uvt = intersect_tri(v0, v1, v2, ro, rd);
                if ((uvt.z > 0) ) {
                    return 0.0;
                }
            } else {
                stack[stack_p++] = node_data.y;
                stack[stack_p++] = node_data.z;

            }

        } 

        if (stack_p == 0.0) {
            return 1.0;
        }
    }
    return 0.0;
}

bool intersect_bvh_tree(vec3 ro, vec3 rd, out intersection inter) {
    vec4 uvt;

    float[20] stack;
    int stack_p = 0;
    stack[stack_p++] = 0.0;

    float node;
    vec3 bb_min;
    vec3 bb_max;
    vec3 node_data;
    float t0, t1;
    vec4 best = vec4(0.0, 0.0, 10000000.0, 0.0);
 
    while (stack_p < 20.0) {
        node = stack[--stack_p];

        bb_min = texture2D(bvh_texture, calc_index_bvh(node, 0.0)).rgb;
        bb_max = texture2D(bvh_texture, calc_index_bvh(node, 1.0)).rgb;
        if (intersect_aabb(ro, rd, bb_min, bb_max, t0, t1) ) {
            node_data = texture2D(bvh_texture, calc_index_bvh(node, 2.0)).rgb;
            if (node_data.x > 0.0) { //node is leaf, intersect tri
                vec3 v0 = texture2D(triangle_texture, calc_index(node_data.x, 0.0, 0.0)).rgb;
                vec3 v1 = texture2D(triangle_texture, calc_index(node_data.x, 1.0, 0.0)).rgb;
                vec3 v2 = texture2D(triangle_texture, calc_index(node_data.x, 2.0, 0.0)).rgb;
                vec4 uvt = intersect_tri(v0, v1, v2, ro, rd);
                if ((uvt.z > 0) && (uvt.z < best.z)) {
                    best = uvt;
                    best.w = node_data.x;
                }
            } else {
                stack[stack_p++] = node_data.z;
                stack[stack_p++] = node_data.y;
                

            }

        } 

        if (stack_p == 0.0) {
            if (best.z < 10000000.0) {
                inter = calc_intersection(best);
                return true;
            } else {
                return false;
            }
        }

    }
    return false;
}

bool intersect_bvh_tree_2(vec3 ro, vec3 rd, out intersection inter) {
    vec4 uvt;

    float[20] stack;
    int stack_p = 0;
    stack[stack_p++] = 0.0;

    float node;
    vec3 bb_min;
    vec3 bb_max;
    vec3 node_data;
    float t0, t1;
    vec4 best = vec4(0.0, 0.0, 10000000.0, 0.0);
 
    while (stack_p < 20.0) {
        node = stack[--stack_p];


        node_data = texture2D(bvh_texture, calc_index_bvh(node, 2.0)).rgb;
        if (node_data.x > 0.0) { //node is leaf, intersect tri
            vec3 v0 = texture2D(triangle_texture, calc_index(node_data.x, 0.0, 0.0)).rgb;
            vec3 v1 = texture2D(triangle_texture, calc_index(node_data.x, 1.0, 0.0)).rgb;
            vec3 v2 = texture2D(triangle_texture, calc_index(node_data.x, 2.0, 0.0)).rgb;
            vec4 uvt = intersect_tri(v0, v1, v2, ro, rd);
            if ((uvt.z > 0) && (uvt.z < best.z)) {
                best = uvt;
                best.w = node_data.x;
            }
        } else {
            bb_min = texture2D(bvh_texture, calc_index_bvh(node, 0.0)).rgb;
            bb_max = texture2D(bvh_texture, calc_index_bvh(node, 1.0)).rgb;
            if (intersect_aabb(ro, rd, bb_min, bb_max, t0, t1) ) {
                stack[stack_p++] = node_data.y;
                stack[stack_p++] = node_data.z;
            }
        } 

        if (stack_p == 0.0) {
            if (best.z < 10000000.0) {
                inter = calc_intersection(best);
                return true;
            } else {
                return false;
            }
        }

    }
    return false;
}



*/
