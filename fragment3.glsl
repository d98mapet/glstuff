#version 330 core
in vec2 uv;

//out vec3 color;
layout(location = 0) out vec3 color;




uniform float iGlobalTime;
uniform float holeSize=1.3; //ui(0.0, 3.0)
////////////////////////////////////////////////////

float sdSphere( vec3 p, float s )
{
  return length(p)-s;
}

float  sdPlaneY(vec3 p) {
    return p.y;
}

float  sdPlaneZ(vec3 p) {
    return p.z;
}
float maxcomp(in vec3 p ) { return max(p.x,max(p.y,p.z));}
float sdBox( vec3 p, vec3 b )
{
  vec3  di = abs(p) - b;
  float mc = maxcomp(di);
  return min(mc,length(max(di,0.0)));
}
float sdCross(vec3 p) {
    float inf = 1000000.0;
    float da = sdBox(p.xyz, vec3(inf, 1.0, 1.0));
    float db = sdBox(p.yzx, vec3(1.0, inf, 1.0));
    float dc = sdBox(p.zxy, vec3(1.0, 1.0, inf));
    return min(da, min(db, dc));
}
float maxcomp(in vec2 p ) { return max(p.x,p.y);}
float sdCross2( in vec3 p )
{
  float da = maxcomp(abs(p.xy));
  float db = maxcomp(abs(p.yz));
  float dc = maxcomp(abs(p.zx));
  return min(da,min(db,dc))-1.0;
}



float sdPlane( vec3 p, vec4 n )
{
  // n must be normalized
  return dot(p,n.xyz) + n.w;
}







uniform float twist = 0.0; //ui(0.0, 2.0);
uniform float split = 3.0; //ui(0.0, 10.0);
float scene(vec3 pos) {
    //float index = floor(pos.x/2.0);
    pos.x = mod(pos.x, 2.0) - 1.0;
    //pos.y = mod(pos.y, 2.0) - 1.0;
    // pos.z = mod(pos.z, 2.0) - 1.0;
    //pos.y = pos.y*sin(pos.x);
    float angle = pos.x*sin(twist);
    pos.y = pos.y*cos(angle) - pos.z*sin(angle);
    pos.z = pos.y*sin(angle) + pos.z*cos(angle);
    vec3 boxpos = pos;//vec3(pos.x, pos.y + 0.1 * sin(3.1415 * (pos.x + iGlobalTime)), pos.z);
    float box = sdBox(boxpos, vec3(1.0, 1.0, 1.0));
    
    //float sph1 = sdSphere(pos, holeSize );
    //sph1 = max(box, -sph1);
    
    float s = 1.0;
    float combine;
    float temp_res = box;
    
    for (int cnt=0; cnt<4; cnt++) {
        vec3 a = mod(pos*s+1.0, 2.0)-1.0;
        
        s*=split;
        vec3 r = split*a;
        float cross = sdCross2(r)/s;
        combine = max(box, -cross);
        if (combine > temp_res) {
            temp_res = combine;
        }
        //combine = min(box, cross);
    }
    return temp_res;
   
    
}

float castRay(vec3 ro, vec3 rd, float eps, float max_t) {
    
    float t=0.15;//scene(ro/1.429);
    
    //float max_t = 40.0;
    //float eps = 0.001;
    float d;
    
    for (int i=0; i<40; i++) {
        vec3 pos = ro+rd*t;
        d = scene(pos);
        if (t<eps || t>max_t) {
            break;
        }
        t += d;  
    }
    if (t>max_t) {
        t = -1.0;
    }
    return t;
}



mat3 setCamera( in vec3 ro, in vec3 ta, float cr )
{
	vec3 cw = normalize(ta-ro);
	vec3 cp = vec3(sin(cr), cos(cr),0.0);
	vec3 cu = normalize( cross(cw,cp) );
	vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}

vec3 calcNormal(in vec3 pos) {
	vec3 eps = vec3( 0.001, 0.0, 0.0 );
	vec3 nor = vec3(
	    scene(pos+eps.xyy) - scene(pos-eps.xyy),
	    scene(pos+eps.yxy) - scene(pos-eps.yxy),
	    scene(pos+eps.yyx) - scene(pos-eps.yyx));
	return normalize(nor);
}




uniform float z_amp = 0.01; // ui(0.0, 1.0)
uniform sampler2D color_map; // path(d:/shaders/red.png)
uniform sampler2D height_map; // path(d:/shaders/clouds.png)



uniform float cx = 4.0; // ui(-10.0, 10.0);
uniform float cy = 0.0; // ui(-10.0, 10.0);
uniform float cz = 0.0; // ui(-10.0, 10.0);


vec2 HmdWarp(vec2 uv)
{
    // screen space transform(Side by Side)
    uv = vec2((mod(uv.x,1.0)-0.5)*2.0+0.2*sign(uv.x), uv.y);

    // HMD Parameters
    vec2 ScaleIn = vec2(1.0);
    vec2 LensCenter = vec2(0.0,0.0);
    vec4 HmdWarpParam = vec4(1.0,0.22, 0.240, 0.00);
    vec2 Scale = vec2(1.0);

    // Distortion
    vec2 theta  = (uv - LensCenter) * ScaleIn; // Scales to [-1, 1]
    float  rSq    = theta.x * theta.x + theta.y * theta.y;
    vec2 rvector= theta * (HmdWarpParam.x + HmdWarpParam.y * rSq
                   + HmdWarpParam.z * rSq * rSq
                   + HmdWarpParam.w * rSq * rSq * rSq);
    return LensCenter + Scale * rvector;
}
float seed = iGlobalTime;
vec2 hash2() {
    
    return fract(sin(vec2(seed+=0.1,seed+=0.1))*vec2(43758.5453123,22578.1459123));
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
    
    return normalize( rr );
}

uniform float time_boost = 0.0; //ui(0.0, 1.0);
uniform float lpx = 1.0; //ui(0.0, 6.0);
uniform float lpz = 1.0; //ui(0.0, 6.0);
uniform float max_dist = 10.0; //ui(1.0, 30.0);
uniform sampler2D input_map;


vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){
   
    n = max(n*n, 0.001); // n = max((abs(n) - 0.2)*7., 0.001); // n = max(abs(n), 0.001), etc.
    n /= (n.x + n.y + n.z ); 
    return (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;
 
    
}

void main()
{
	color = vec3(0.0, 0.2, 0.7);
    vec3 lightPos = vec3(60.0*sin(lpx), 60.0, 60.0*cos(lpz));

    //vec3 ro = vec3( cx, cy, cz);
    vec3 ro = vec3( 0.0+iGlobalTime*time_boost, 0.0, 0.0);
    vec3 ta = vec3( cx+iGlobalTime*time_boost, cy, cz);//cos(iGlobalTime), sin(iGlobalTime) );
    vec2 p = -1.0+2.0*uv;

    /*p=HmdWarp(p);

    if(abs(p.x)>1.0 || abs(p.y) > 1.0) {
        color = vec3(0.0);//texture(color_map, uv).rgb;
        return;
        //discard;
    }
*/
    seed = p.x + p.y * 3.43121412313 + iGlobalTime;
    ro.x = ro.x + 0.01*sin(seed+p.x+p.y);
    mat3 cam_to_world = setCamera(ro, ta, 0.0);
    vec3 rd = cam_to_world * normalize( vec3(p.xy,1.0) );
    rd = normalize(rd);
    float s = 0.0;
    vec3 o_ro = ro;
    vec3 o_rd = rd;
    color = vec3(0.0,0.0,0.0);
    
    for (int samples = 0; samples<1; samples++) {
        float d = 0.0;
        ro = o_ro;
        rd = o_rd;
        vec3 mask = vec3(1.0);
        for (int depth=0; depth<5; depth++) {
            float t = castRay(ro, rd, .01, max_dist);
            if (t>0.0) {
                vec3 inter = vec3(ro+rd*t);
                vec3 normal = calcNormal(inter);
                //color = normal*.5+0.5;
                //return;
                //color = vec3(t/max_dist);
                //return;
                vec3 lightDir = lightPos-inter;
                float light_dist = length(lightDir);
                lightDir = normalize(lightDir);

                float t_light = castRay(inter+lightDir*light_dist, -lightDir, 0.1, light_dist+1.0);
                float diffuse = dot(lightDir, normal);
                /*t_light<0.0 || t_light > light_dist) {*/
                vec3 lc; 
                if ((abs(light_dist-t_light)<0.01)) { 
                    //if (normal.y > .9) {            
                        lc = vec3(diffuse*1.0);// * tex3D(color_map, inter, normal);
                    //}  else {
                       // color += vec3(diffuse*1.0);
                    //}
                } else {
                    lc = vec3(diffuse*0.0);
                }
                color += mask*tex3D(color_map, inter/4.0, normal)*lc;
                mask *= tex3D(color_map, inter, normal);
                //return;
                ro = inter;//+normal*.01;
                rd = normalize(cosWeightedRandomHemisphereDirection(normal));
                d++;

            } else {
                color += vec3(0.0,0.0,0.7)*mask;
                d++;
                //s++;
                break;
            }
            
        } 
 
        s++;
    }
    color /= s;
    
    if (iGlobalTime>0.0) {
        color = (color + (texture(input_map, uv).rgb));
    }
    
}
