#version 330 core

in vec2 TexCoord;

uniform vec2 WindowSize;
uniform vec2 MousePos;
uniform vec3 cameraPos;
uniform float time;
uniform float scaleCoeff;
uniform vec2 cameraRadian;

uniform mat4 modelMat;

out vec4 FragColor;

#define PI 3.14159265359

vec3 fogColor = vec3(0.2, 0.1, 0.7); // Color of the fog
vec3 skyColor = fogColor * 0.9f; // Color of the fog
float fogDensity = 0.02;               // Density of the fog
float fogStartDistance = 10.0;        // Distance at which fog begins

float degreesToRadians(float degrees) {
    return degrees * PI / 180.0;
}

mat3 rotateX(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(1, 0, 0),
        vec3(0, c, -s),
        vec3(0, s, c)
    );
}

mat3 rotateY(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, 0, s),
        vec3(0, 1, 0),
        vec3(-s, 0, c)
    );
}

mat3 rotateZ(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, -s, 0),
        vec3(s, c, 0),
        vec3(0, 0, 1)
    );
}

float distance_from_sphere(vec3 p , vec3 c , float r)
{
    return length(p - c) - r;
}

float fPlane(vec3 p , vec3 n , float distance_from_origin)
{
    return dot(p,n) + distance_from_origin;
}

float vmax(vec3 v) {
	return max(max(v.x, v.y), v.z);
}

// Box: correct distance to corners
float fBox(vec3 p, vec3 b , vec3 BoxPosition) {
    p -= BoxPosition;
	vec3 d = abs(p) - b;
	return length(max(d, vec3(0))) + vmax(min(d, vec3(0)));
}

// Capsule: A Cylinder with round caps on both sides
float fCapsule(vec3 p, float r, float c , vec3 capsulePosition) {
    p -= capsulePosition;
	return mix(length(p.xz) - r, length(vec3(p.x, abs(p.y) - c, p.z)) - r, step(c, abs(p.y)));
}

float fCylinder(vec3 p, float r, float height) {
	float d = length(p.xz) - r;
	d = max(d, abs(p.y) - height);
	return d;
}

float IntersectSDF(float dist1 , float dist2)
{
    return max(dist1,dist2);
}

float UnionSDF(float dist1 , float dist2)
{
    return min(dist1,dist2);
}

float DifferenceSDF(float dist1 , float dist2)
{
    return max(dist1,-dist2);
}

vec4 map_the_world(vec3 p)
{
    float displacement = sin(5.0f * p.x) * sin(5.0f * p.y) * sin(5.0f * p.z) * 0.25;
    float sphere0 = distance_from_sphere(p,vec3(0.0f),1.0f);
    float plane0 = fPlane(p,vec3(0.0f,1.0f,0.0f),1.0f);
    float Capsule0 = fCapsule(p,0.8f,1.0f , vec3(-3.0f,1.0f,0.0f));
    float Box0 = fBox(p , vec3(1.0f),vec3(-3,0.0f,0.0f));

    float Cylinder0 = fCylinder(p * inverse(rotateX(degreesToRadians(90))) - vec3(-3,0.0f,0.0f) ,0.8f,1.5f);


    float finalObject = DifferenceSDF(Box0,Capsule0);
    finalObject = DifferenceSDF(finalObject,Cylinder0);

    float OutObject = min(min(plane0 ,finalObject),sphere0);
    vec3 ObjectColor = vec3(1.0f,0.0f,1.0f);

    if(OutObject >= finalObject)
    {
        ObjectColor = vec3(217/255.0f,113/255.0f,168/255.0f);
    }
    else if(OutObject >= sphere0)
    {
        ObjectColor = vec3(255/255.0f,223/255.0f,13/255.0f);
    }

    return vec4(OutObject,ObjectColor);

}

vec3 calculate_normal(vec3 p)
{
    const vec3 small_step = vec3(0.001f,0.0f,0.0f);

    float gradient_x = map_the_world(p+small_step.xyy).x - map_the_world(p-small_step.xyy).x;
    float gradient_y = map_the_world(p+small_step.yxy).x - map_the_world(p-small_step.yxy).x;
    float gradient_z = map_the_world(p+small_step.yyx).x - map_the_world(p-small_step.yyx).x;

    vec3 normal = vec3(gradient_x,gradient_y,gradient_z);
    return normalize(normal);
}


vec3 Get_Shading(vec3 current_position,vec3 ro , vec3 rd ,vec3 ObjectColor)
{
    vec3 normal = calculate_normal(current_position);
          
    vec2 normalized_mouse_pos = (MousePos / WindowSize) * 2.0f - 1.0f;
    vec3 light_position = vec3(sin(time),4.0f,cos(time));

    vec3 direction_to_light = normalize(light_position - current_position);
    float diffuse_intensity = max(0.0f,dot(normal,direction_to_light));

    float amb = 0.5 + 0.5 * dot(normal, vec3(0., 1., 0.));

    float specularlight = 0.20f;
    vec3 viewdirection = normalize(ro - current_position);
    vec3 halfwaydir = normalize(direction_to_light + viewdirection);

    float specular_amount = pow(max(dot(normal,halfwaydir),0.0f),32.0);
    float specular = specular_amount * specularlight;

    vec3 color = fogColor;
    vec3 ambient = color * 0.1f;
    vec3 fresnel = 0.25 * color * pow(1.0 + dot(rd, normal), 3.0);

    const int NUMBER_OF_SHADOW_STEPS = 32; // Number of steps for shadow ray marching
    const float SHADOW_STEP_SIZE = 0.1;   // Step size for shadow ray marching
    const float MINIMUM_SHADOW_HIT_DISTANCE = 0.001; // Minimum distance for shadow hits

    float shadow = 1.0; 
    vec3 light_direction = normalize(light_position - current_position);

    vec3 shadow_ray_origin = current_position + 0.05 * light_direction; 
    vec3 shadow_ray_direction = light_direction;

    for (int j = 0; j < NUMBER_OF_SHADOW_STEPS; ++j)
    {
        vec3 shadow_ray_position = shadow_ray_origin + float(j) * SHADOW_STEP_SIZE * shadow_ray_direction;
        vec4 map = map_the_world(shadow_ray_position);
        float shadow_distance = map.x;

        if (shadow_distance < MINIMUM_SHADOW_HIT_DISTANCE)
        {
            shadow = 0.5;
            break; 
        }
    }

    vec3 shaded_color = (amb * (ObjectColor / 2) + (fresnel * 0.8f) + ObjectColor * diffuse_intensity + specular) * shadow + ((1.0f-shadow) * ambient);

    if(current_position.z > fogStartDistance)
    {
        float fogAmount = 1.0f - exp(-fogDensity * (current_position.z - fogStartDistance));
        shaded_color = mix(shaded_color, fogColor, fogAmount);
    }

    return shaded_color;
}

vec3 ray_march(vec3 ro , vec3 rd , vec3 ObjectColor)
{
    float total_distance_traveled = 0.0f;
    const int NUMBER_OF_STEPS = 512;
    const float MINIMUM_HIT_DISTANCE = 0.001f;
    const float MAXIMUM_TRACE_DISTANCE = 1000.0f;

    for (int i = 0; i < NUMBER_OF_STEPS; ++i)
    {
        vec3 current_position = ro + total_distance_traveled * rd;
        vec4 map = map_the_world(current_position);
        float distance_to_closest = map.x;

        if(distance_to_closest < MINIMUM_HIT_DISTANCE)
        {
            return Get_Shading(current_position,ro,rd,map.yzw);
        }
        if(total_distance_traveled > MAXIMUM_TRACE_DISTANCE)
        {
            break;
        }

        total_distance_traveled += distance_to_closest;
    }

    vec3 OutsideColor = skyColor * (vec3(1.0 - gl_FragCoord.y / WindowSize.y) * vec3(2.0f) + 0.3f);

    return OutsideColor;
}

void main()
{
    vec2 uv = (gl_FragCoord.xy / WindowSize) * 2.0f - 1.0f;
    uv.y *= scaleCoeff; 
    vec3 camera_position = vec3(0.0f,0.0f,-5.0f);
    
    mat3 directionMat = rotateX(degreesToRadians(cameraRadian.y)) * rotateY(degreesToRadians(cameraRadian.x)) * rotateZ(0.0);
    vec3 ro = camera_position + (cameraPos * directionMat);
    vec3 rd = normalize(vec3(uv, 1.0f) * directionMat);

    vec3 shaded_color = ray_march(ro,rd,vec3(1.0f,0.0f,1.0f));

    shaded_color = pow(shaded_color,vec3(0.4545));
    FragColor = vec4(shaded_color, 1.0);
}
