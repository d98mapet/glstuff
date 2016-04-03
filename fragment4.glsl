#version 330 core
in vec2 uv;
layout(location = 0) out vec3 color;

uniform float iGlobalTime;
uniform float cx = 4.0; // ui(-10.0, 10.0);
uniform float cy = 0.0; // ui(-10.0, 10.0);
uniform float cz = 0.0; // ui(-10.0, 10.0);

uniform usamplerBuffer bvh_tree;

float raysphereintersect(vec3 orig, vec3 dir, vec3 center, float radius2) {
    vec3 L = center - orig;
    float tca = dot(L, dir);
    if (tca < 0) return -1.0;
    float d2 = dot(L, L) - tca * tca;
    if (d2 > radius2) return -1.0;
    float thc = sqrt(radius2 - d2);
    float t0 = tca - thc;
    float t1 = tca + thc;
    if (t0 < t1) {
        if (t0 < 0) return t1;
        return t0;        
    } else {
        if (t1 < 0) return t0;
        return t1;
    }
}

vec3 spherenormal(vec3 orig, vec3 dir, vec3 center, float sphereradius2, float t_hit) {
    vec3 pos = orig + dir * t_hit - center;
    return normalize(pos);
}

vec3 normal2color(vec3 normal) {
    return normal * 0.5 + 0.5;
}

vec4 bvhintersect_buf(vec3 orig, vec3 dir) {
    int i = 0;
    vec4 best_intersect = vec4(0, 0, 0, 99999);

    for (int j = 0; j < 15; ++j) {
        uvec4 u_bvh1 = texelFetch(bvh_tree, i * 2);
        uvec4 u_bvh2 = texelFetch(bvh_tree, i * 2 + 1);
        vec4 bvh1 = uintBitsToFloat(u_bvh1);
        vec4 bvh2 = uintBitsToFloat(u_bvh2);
        int next = int(bvh1.w);
        if (next > 0) {
            vec3 bvh_min = bvh1.xyz;
            vec3 bvh_max = bvh2.xyz;
            vec3 min_trans = (bvh_min - orig) / dir;
            vec3 max_trans = (bvh_max - orig) / dir;
            vec3 t0 = min(min_trans, max_trans);
            vec3 t1 = max(min_trans, max_trans);
            float tmin = max(max(t0.x, t0.y), t0.z);
            float tmax = min(min(t1.x, t1.y), t1.z);
            if (tmax <= tmin) {
                int skip = int(bvh2.w);
                if (skip == 0) return best_intersect;
                i += skip;
            } else {
                i += next;
            }
        } else if (next < 0) {
            vec3 center = bvh1.xyz;
            float radius = bvh2.x;
            float radius2 = radius * radius;
            float t = raysphereintersect(orig, dir, center, radius2);
            if (t > 0) {
                if (t < best_intersect.w) {
                    vec3 normal = spherenormal(orig, dir, center, radius2, t);
                    best_intersect = vec4(normal, t);
                }
            }
            i -= next;
        } else {
            return best_intersect;
        }
    };
    return best_intersect;
}

void orthocamera(vec2 uv, float zoom, out vec3 orig, out vec3 dir) {
    orig = vec3(zoom * (uv.x - 0.5), -20, zoom * (uv.y - 0.5));
    dir = vec3(0, 1, 0);
}

#define M_PI 3.1415926535897932384626433832795

mat3 set_camera(in vec3 ro, in vec3 ta, float cr) {
    vec3 cw = normalize(ta - ro);
    vec3 cp = vec3(sin(cr), cos(cr), 0.0);
    vec3 cu = normalize(cross(cw, cp));
    vec3 cv = normalize(cross(cu, cw));
    return mat3(cu, cv, cw);
}

void perspcamera(vec2 uv, float width, float height, float fov, in vec3 orig, in vec3 lookat, out vec3 dir) {
    float iar = width / height;
    float t = tan(fov / 2 * M_PI / 180);
    float px = (2 * uv.x - 1) * t * iar;
    float py = (1 - 2 * uv.y) * t;
    mat3 lookat_mat = set_camera(orig, lookat, 0);
    dir = lookat_mat * normalize(vec3(px, py, 1));
}

void main() {
    float alpha = iGlobalTime / 100;
    vec3 rayorig = 3 * vec3(sin(alpha), -0.5, cos(alpha));
    vec3 raydir;
    vec3 lookat = vec3(0, 0, 0);
    perspcamera(uv, 640, 480, 90, rayorig, lookat, raydir);

    vec4 t_hit = bvhintersect_buf(rayorig, raydir);
    if (t_hit.w > 0) {
        color = normal2color(t_hit.xyz);
    } else {
        color = vec3(1, 0, 0);
    }
}
