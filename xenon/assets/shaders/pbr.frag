#version 460 core

//---------------------------------------------------------------
// [SECTION] Input data & output variables
//---------------------------------------------------------------

in vec4 position;	// Frag pos
in vec3 normal;		// Frag normal
in vec3 tangent;
in vec2 textureCoord;

in mat3 TBN;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out int fragObjectID;


//---------------------------------------------------------------
// [SECTION] Constants
//---------------------------------------------------------------

const float PI = 3.14159265359;

//---------------------------------------------------------------
// [SECTION] Material
//---------------------------------------------------------------

uniform vec4 baseColorFactor;
uniform float metallicFactor;
uniform float roughnessFactor;
uniform vec3 emissiveFactor;  // TODO: implement
uniform int alphaMode;  // TODO: implement
uniform float alphaCutoff;
uniform bool doubleSided;  // TODO: implement

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D metallicRoughnessMap;
layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 3) uniform sampler2D aoMap;
layout(binding = 4) uniform sampler2D emissiveMap;  // TODO: implement

uniform bool usingAlbedoMap;
uniform bool usingMetallicRoughnessMap;
uniform bool usingNormalMap;
uniform bool usingAOMap;
uniform bool usingEmissiveMap;  // TODO: implement

uniform bool usingAttribTangent;
uniform bool usingAttribNormal;
uniform bool usingAttribTexCoord0;
uniform bool usingAttribTexCoord1;  // TODO: implement
uniform bool usingAttribColor0;  // TODO: implement


//---------------------------------------------------------------
// [SECTION] Environment
//---------------------------------------------------------------

layout(binding = 5) uniform samplerCube irradianceMap;
layout(binding = 6) uniform samplerCube radianceMap;
layout(binding = 7) uniform sampler2D brdfLUT;


//---------------------------------------------------------------
// [SECTION] Camera
//---------------------------------------------------------------

struct Camera {
	vec3 position;
	vec3 direction;
};

uniform Camera camera;


//---------------------------------------------------------------
// [SECTION] Picking
//---------------------------------------------------------------

uniform int objectID;


//---------------------------------------------------------------
// [SECTION] Point lights
//---------------------------------------------------------------

#define MAX_POINT_LIGHTS 4
#define EPSILON 0.0000001

struct PointLight {
	vec3 position;
	vec3 color;
};

uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int pointLightsUsed;


//---------------------------------------------------------------
// [SECTION] Trowbridge-Reitz GGX normal distribution function
//---------------------------------------------------------------

float distributionGGX(float NdotH, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;
	return a2 / max(denom, EPSILON);
}


//---------------------------------------------------------------
// [SECTION] Smith's method using Schlick-GGX geometry function
//---------------------------------------------------------------

float geometrySmith(float NdotV, float NdotL, float roughness) {
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
	float ggx2 = NdotL / (NdotL * (1.0 - k) + k);
	return ggx1 * ggx2;
}


//---------------------------------------------------------------
// [SECTION] Fresnel-Schlick
//---------------------------------------------------------------

vec3 fresnelSchlick(float HdotV, vec3 baseReflectivity) {
	// baseReflectivity in range 0 to 1
	// returns range of baseReflectivity to 1
	// increases as HdotV decreses (more reflectivity when surface viewed at large angles)
	return baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - HdotV, 5.0);
}


//---------------------------------------------------------------
// [SECTION] Fresnel-Schlick Roughness
//---------------------------------------------------------------

vec3 fresnelSchlickRoughness(float HdotV, vec3 baseReflectivity, float roughness) {
	return baseReflectivity + (max(vec3(1.0 - roughness), baseReflectivity) - baseReflectivity) * pow(1.0 - HdotV, 5.0);
}


//---------------------------------------------------------------
// [SECTION] Fresnel-Schlick
//---------------------------------------------------------------

vec3 getNormal() {
	vec3 normalMapNormal = normal;

	if(usingNormalMap) {
		vec3 tangentNormal = texture(normalMap, textureCoord).rgb * 2.0 - 1.0;
		
		if(!usingAttribTangent) {
			// TODO: Deprecate
			vec3 Q1  = dFdx(position.xyz);
			vec3 Q2  = dFdy(position.xyz);
			vec2 st1 = dFdx(textureCoord);
			vec2 st2 = dFdy(textureCoord);

			vec3 N   = normalize(normal);
			vec3 T   = normalize(Q1*st2.t - Q2*st1.t);
			vec3 B   = -normalize(cross(N, T));
			normalMapNormal = normalize(mat3(T, B, N) * tangentNormal);
		}
		normalMapNormal = normalize(TBN * tangentNormal);
	}

	return normalMapNormal;
}


//---------------------------------------------------------------
// [SECTION] Main program
//---------------------------------------------------------------

void main() {

	vec3 N = normalize(getNormal());
	vec3 V = normalize(camera.position - position.xyz);
	vec3 R = reflect(-V, N);
	float NdotV = max(dot(N, V), EPSILON);

	// Extract material data
	vec4 albedo = baseColorFactor;
	if(usingAlbedoMap) {
		albedo = albedo * texture(albedoMap, textureCoord);
	}

	if(albedo.a < alphaCutoff) {
		discard;
	}

	float metallic = metallicFactor;
	float roughness = roughnessFactor;
	if(usingMetallicRoughnessMap) {
		vec4 metallicRoughness = texture(metallicRoughnessMap, textureCoord);
		metallic = metallic * metallicRoughness.b;
		roughness = roughness * metallicRoughness.g;
	}

	float ao = 1.0f;
	if(usingAOMap) {
		ao = texture(aoMap, textureCoord).r;
	}

	vec3 emission = vec3(0);
	if(usingEmissiveMap) {
		emission = texture(emissiveMap, textureCoord).rgb * emissiveFactor;
	}
	
	// calculate reflectance at normal incidence; if dia-electric (like plastic) use baseReflectivity
	// of 0.04 and if it's a metal, use the albedo color as baseReflectivity (metallic workflow)
	vec3 baseReflectivity = mix(vec3(0.04), albedo.rgb, metallic);

	// reflectance equation
	vec3 Lo = vec3(0.0);
	for(int i = 0; i < pointLightsUsed; ++i) {
		PointLight light = pointLights[i];
		
		// calculate per-light radiance
		vec3 L = normalize(light.position - position.xyz);
		vec3 H = normalize(V + L);
		float dist = length(light.position - position.xyz);
		float attenuation = 1.0 / (dist * dist);
		vec3 radiance = light.color * attenuation;

		// Cook-Torrance BRDF
		float NdotL = max(dot(N, L), EPSILON);
		float HdotV = max(dot(H, V), 0.0);
		float NdotH = max(dot(N, H), 0.0);

		float D = distributionGGX(NdotH, roughness);  // larger the more micro-facets aligned to H (normal distribution function)
		float G = geometrySmith(NdotV, NdotL, roughness);  // smaller the more micro-facets shadowed by other micro-facets
		vec3 F = fresnelSchlick(HdotV, baseReflectivity);  // proportion of specular reflectance

		vec3 specular = D * G * F;
		specular /= 4.0 * NdotV * NdotL;

		// for energy conservation, the diffuse and specular light can't be
		// above 1.0 (unless the surface emits light); to preserve this
		// relationship the diffuse component (kD) shoudl equal 1.0 - kS.
		vec3 kD = vec3(1.0) - F;  // F equal kS

		// multiply kD by the inverse metalness such that only non-metals
		// have diffuse lightning, or a linear blend if partly metal
		// (pure metals have no diffuse light)
		kD *= 1.0 - metallic;

		// note that 1) angle of light to surface affects specular, not just diffuse
		//           2) we mix albed with diffuse, but not specular
		Lo += (kD * albedo.rgb / PI + specular) * radiance * NdotL;
	}

	// IBL ambient lightning
	vec3 F = fresnelSchlickRoughness(NdotV, baseReflectivity, roughness);
	vec3 kD = (1.0 - F) * (1.0 - metallic);

	vec3 irradiance = texture(irradianceMap, N).rgb;
	vec3 diffuse = irradiance * albedo.rgb;


	const float MAX_REFLECTION_LOD = 4.0;
	vec3 radianceColor = textureLod(radianceMap, R, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 brdf = texture(brdfLUT, vec2(NdotV, roughness)).rg;
	vec3 specular = radianceColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular) * ao;
	
	vec3 color = ambient + Lo + emission;


	// HDR tonemapping
	color = color / (color + vec3(1.0));
	// gamma correction
	color = pow(color, vec3(1.0/2.2));


	fragColor = vec4(color, 1.0);

	fragObjectID = objectID;
}
